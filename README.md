
# FastArraySerializer Bug

  

This project is an example to showcase a bug in FastArraySerializer where the FastArraySerializer property will stop replicating, in the log it shows:

  

>LogNet: Error: FRepLayout::ReceiveCustomDeltaProperty: NetDeltaSerialize - Mismatch read. Property: TestFastArray, Object: BP_GrandChildActor_C /Game/UEDPIE_0_TestFastArrayMap.TestFastArrayMap:PersistentLevel.BP_GrandChildActor_C_0

  
  

## Repro Steps

  

- Download and compile the source

- Run the TestFastArrayMap level with a single player with NetMode **Client** and make sure under Advanced Settings that **Run Under One Process** is NOT checked.

- After 3 seconds an instance BP_ChildActor will spawn and 3 seconds later an instance of BP_GrandChildActor will spawn. In the log you will see:

> LogNet: Error: FRepLayout::ReceiveCustomDeltaProperty: NetDeltaSerialize - Mismatch read. Property: TestFastArray, Object: BP_GrandChildActor_C /Game/UEDPIE_0_TestFastArrayMap.TestFastArrayMap:PersistentLevel.BP_GrandChildActor_C_0 in

  

## How it works

- Base C++ class (ie **Parent**) with a replicated FastArraySerializer property

- On the server, put some starting data inside that array before any replication occurs

- Have a blueprint child class (**Child**), and another child blueprint of that blueprint (**GrandChild**)

- Have soft class references to both blueprints

- Spawn the **Child** blueprint first

- Spawn the **GrandChild** blueprint next

- See in the log that things broke

  

## What is going wrong?

  

### On the Server:

1. When **Child** is first spawned it creates a replicator for the **Child** actor and runs

-  `UNetConnection::CreateReplicatorForNewActorChannel`

-  `FObjectReplicator::InitWithObject` where it chooses **Childs**  **CDO** as the Source Object

- ...

-  `FFastArraySerializer::NetDeltaSerialize`

- At this point, `ArraySerializer.DeltaFlags` is `EFastArraySerializerDeltaFlags::HasDeltaBeenRequested (2)` which is the default value.

-  `ArraySerializer.DeltaFlags` then has `EFastArraySerializerDeltaFlags::HasBeenSerialized` and `EFastArraySerializerDeltaFlags::IsUsingDeltaSerialization` OR'd onto it for a value of 7.

- REMEMBER we are modifying the ArraySerializer on **Childs**  **CDO** since it was chosen as the source object.

-  *Result*: **Childs**  **CDO**  `ArraySerializer.DeltaFlags` property = 7

  

2. When **GrandChild** is spawned it first has to create the **GrandChild CDO**

-  **GrandChilds CDO** uses **Childs CDO** as its archetype during creation.

- Remember **Childs CDO**  `ArraySerializer.DeltaFlags` is set to 7 so now **GrandChilds CDO**  `ArraySerializer.DeltaFlags` is set also to 7.

-  *Result*: **GrandChilds**  **CDO**  `ArraySerializer.DeltaFlags` property = 7

  

3.  **GrandChilds CDO** is re-instanced and recompiled in `UBlueprintGeneratedClass::ConditionalRecompileClass`

- This calls `FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects` where the `OldObject` is the previous **GrandChild CDO** (with value 7) and `NewObject` is the new **GrandChild CDO** that has just been instanced.

-  `Serialize` is called and each property is saved one at a time from **OLD GrandChild CDO** to be loaded into **NEW Grandchild CDO**

-  `ArraySerializer.DeltaFlags` is of type `EFastArraySerializerDeltaFlags` with values `None (0)`, `HasBeenSerialized (1)`, `HasDeltaBeenRequested (2)`, and `IsUsingDeltaSerialization (7)`

- In `FEnumProperty::SerializeItem` it tries to serialize `ArraySerializer.DeltaFlags` as the value 7 which is not a valid enum value, and thus saves an empty FName which saves to the string `None`.

- When **GrandChilds CDO** comes to de-serialize properties, `FEnumProperty::SerializeItem` sees the `None` value for `DeltaFlags` which happens to be a valid value for this enum and selects the value `0`.

-  **GrandChilds CDO** now has the property `ArraySerializer.DeltaFlags` with value `EFastArraySerializerDeltaFlags::None (0)`

-  *Result*: **GrandChilds**  **CDO**  `ArraySerializer.DeltaFlags` property = 0

4. The actual instance of **GrandChild** is spawned and copies its `ArraySerializer.DeltaFlags` property is copied from its CDO and has value `None (0)`.

- A replicator for this **GrandChild** is created (like in Step 1) and it calls into `FFastArraySerializer::FastArrayDeltaSerialize`.

- Since its `ArraySerializer.DeltaFlags` is `None (0)` and does not have `EFastArraySerializerDeltaFlags::HasDeltaBeenRequested`, the `EFastArraySerializerDeltaFlags::IsUsingDeltaSerialization` is never OR'd onto `DeltaFlags` it and **instead of calling into `FastArrayDeltaSerialize_DeltaSerializeStructs`** the function continues and the data is written out at the bottom of that function (`FastArrayDeltaSerialize()`).

-  *Result*: **GrandChild** is serialized on the server not using `FastArrayDeltaSerialize_DeltaSerializeStructs`

### On the Client:

1. When **Child** and **GrandChild** is replicated from the server it has to create CDOs for them.

- However, it never calls `FFastArraySerializer::FastArrayDeltaSerialize` where `Parms.bIsInitializingBaseFromDefault` since the client is just receiving data.

- Therefore the 7 value is never serialized in a CDO, thereby causing it to deserialize as 0.

- When the client goes to deserialize **GrandChild** for the first time inside of `FFastArraySerializer::FastArrayDeltaSerialize` its `ArraySerializer.DeltaFlags` is set to `HasDeltaBeenRequested (2)`. This causes it to have `EFastArraySerializerDeltaFlags::HasBeenSerialized` and `EFastArraySerializerDeltaFlags::IsUsingDeltaSerialization` OR'd onto it, and calls in `FastArrayDeltaSerialize_DeltaSerializeStructs`  **UNLIKE THE SERVER WHEN IT SERIALIZED**

- *Result*: **GrandChild** is de-serialized on the client using `FastArrayDeltaSerialize_DeltaSerializeStructs`

2. Inside `FRepLayout::ReceiveCustomDeltaProperty` the `Parmas.Reader` on the client has leftover data leading to

>UE_LOG(LogNet, Error, TEXT("FRepLayout::ReceiveCustomDeltaProperty: NetDeltaSerialize - Mismatch read. Property: %s, Object: %s"), *Params.DebugName, *Params.Object->GetFullName());

  

## The Fix

  ### Dont modify ArraySerializer.DeltaFlags on any CDO inside FFastArraySerializer::FastArrayDeltaSerialize

- Inside `FFastArraySerializer::FastArrayDeltaSerialize`, we can detect if we're operating on a CDO because `Parms.bIsInitializingBaseFromDefault` will be true. If this is true, we shouldn't modify the `DeltaFlags` property.
- Other places in that same function that use `bIsInitializingBaseFromDefault` say 
>// When we are initializing from default we use a simplified variant of BuildChangedAndDeletedBuffers that does not modify the source state` which leads me to believe we shouldnt modify the source object
  - Also, the DeltaFlags property is marked `UPROPERTY(NotReplicated, Transient)`. Its my belief is that it was intended to NOT be serialized between objects. However, if you look at the definition of `CPF_Transient` it says 
  > `///< Property is transient: shouldn't be saved or loaded, except for Blueprint CDOs.`
  >
   which is our exact situation.
