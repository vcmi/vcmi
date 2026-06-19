# Artifact

A static artifact definition from the game database. Identifies an artifact kind; concrete instances worn by heroes are a separate concept not currently exposed to scripts.

### getJsonKey

Returns the JSON key (mod-scoped identifier) of this entity.

- returns `string`

### isBig

True if the artifact occupies the 'big' artifact slot (cannot be carried in the backpack).

- returns `boolean`

### isTradable

True if the artifact can be traded between heroes or sold at the marketplace.

- returns `boolean`

### getPrice

Returns the artifact's gold value.

- returns `integer`
