# Composite Army Display

Adds a neutral adventure-map object that can contain up to seven creature
stacks.

The object behaves like a wandering monster:

- it is always neutral and can not be owned by a player;
- it is hostile on contact despite having a neutral owner;
- visiting a non-empty army starts a battle;
- the representative creature is selected by stack count divided by weekly
  growth;
- the adventure-map animation uses the representative creature;
- a neutral star marks the object as a composite army;
- right-clicking displays every creature stack.

This object does not implement desertion, recovery, supply costs, lobby
options, or weekly simultaneous turns.
