# Component

Descriptor for an icon shown in a message window (a creature, artifact, resource, skill, ... with an optional amount). `type` selects the kind and `subType` the specific entity.

### type

Component kind (ComponentType index).

- type: `integer`

### subType

Identifier index of the entity, interpreted according to `type`.

- type: `integer`

### value

Optional amount: positive means gained, negative means lost.

- type: `integer?`
