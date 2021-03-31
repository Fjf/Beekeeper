## Hive Engine


### Known bugs (haha):

#### The _fits_ rule for stacked beetles.
The first rule is where tiles can only move to a place if they can physically fit through any gaps to get there, is not implemented completely.
This situation is when multiple beetles are on top of the hive, and two beetles block a third beetle from moving through.
This is not implemented because the Hive engine does not keep track of a _z-axis_.
As there are 4 beetles, the maximum height is 5.
If all possible positions of a beetle would be tracked, the game board array would need to be 22x22x5.
This is a huge waste of space, so instead we keep a _stack-tracker_ in memory, which tracks tiles below beetles.
We do a lookup every time a beetle gets moved to a new position to replace the original tile with the tile in the stack.
This does mean that for z-axis of 1, movement does not get blocked correctly.


### Zobrist Hash table initialization
When running the python DLL, ensure you initialized the Zobrist hash table first.