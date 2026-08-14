# MapSetup

Setup handle given to a map script's init method. Lets the script list the map's objects and bind its handler functions to event objects by their instance name.

### attachEventScript

Binds one of the script's handler functions to a map event object: visiting that object then runs the named function (with the usual game, server, object, hero arguments) instead of the object's default reward. Errors if no object has that instance name or the object is not an event/pandora.

- param `funcName`: `string` — Name of the script function to run when the object is visited.
- param `objectName`: `string` — Instance name of the event/pandora object to bind the handler to.

### objects

Returns all objects on the map so the script can find the ones to attach handlers to (match on getInstanceName).

- returns [`MapObject[]`](MapObject.md) — Every object currently placed on the map.
