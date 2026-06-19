# MetaString

Object for constructing strings with translation support. Supports appending text via `append` field and `replaceStrings` / `replaceNumbers` to fill the %s and %d placeholders. Used in all places that expect translatable string with formattingsuch as battle log and spell-problem messages. In case of multiplayer, string is translated on each client according to their language settings

### append

Sequence of text-ID tokens to concatenate.

- type: `string[]`

### replaceStrings

Values that fill %s placeholders in the appended tokens, in order.

- type: `string[]`

### replaceNumbers

Values that fill %d placeholders in the appended tokens, in order.

- type: `integer[]`
