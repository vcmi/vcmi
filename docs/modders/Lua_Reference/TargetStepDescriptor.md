# TargetStepDescriptor

Cursor and status bar texts that the game shows while the player chooses one destination of a spell target. Returned by the `describeTargetStep` function of a spell effect script.

### cursor

Identifier of the cursor from `config/cursors.json` shown over a hex that is valid for the destination.

- type: `string`

### text

Status bar text for a hex that is valid for the destination.

- type: [`MetaString`](MetaString.md)

### blockedText

Status bar text for a hex that is not valid for the destination. No text if not set.

- type: [`MetaString`](MetaString.md)

### healthGainTextID

If set, the health change of the spell at the hovered hex is appended to `text` in brackets. A health gain uses the plural forms of this text ID.

- type: `string`
