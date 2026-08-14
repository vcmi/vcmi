# MapObject

A handle to an adventure-map object (event, town, monster, resource, ...). Right now used only to identify the object a trigger fired on, so scripts can act on it (e.g. remove it).

### getOwner

Returns the owner of this map object.

- returns `integer` — Player that owns this object, or the neutral player when unowned.

### getInstanceName

Returns the map-unique instance name identifying this object.

- returns `string` — The object's unique instance name, as set in the map editor or auto-generated.
