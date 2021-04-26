import re


def letter_to_name(letter):
    if letter == "Q":
        return "QUEEN"
    if letter == "G":
        return "GRASSHOPPER"
    if letter == "S":
        return "SPIDER"
    if letter == "B":
        return "BEETLE"
    if letter == "A":
        return "ANT"


def tile_to_engine_notation(tile):
    color = "L" if tile[0] == "w" else "D"
    tile_type = letter_to_name(tile[1])

    return_type = "%s_%s" % (color, tile_type)
    number = tile[2:] if len(tile) > 2 else 1
    return return_type, int(number)


def main():
    input_string = "bG1+0+1bQ+0+2wS2+0+3bA3+0-1bB1+0-2wA3+0-3wG1+1+0wB2wB1+1+2wA2+1-3wS1+2+0bA2+3-1bS1+3-2wG2+4-2bB2wQ-1+0bA1-1+1bG2-1-1wA1-2+1bG3-3+1*@bS*@wG"

    result = re.findall(r"((([bw][AGBQS][123]?)+)([+-]\d+)([+-]\d+))", input_string)

    collection = []
    for _, tiles, _, x, y in result:
        tiles = [tile for tile in re.findall(r"[bw][AGBQS][123]?", tiles)]

        collection.append([tiles, int(x), int(y)])

    min_x = 1000
    min_y = 1000
    for item in collection:
        item[1] += item[2]
        _, x, y = item
        if x < min_x:
            min_x = x
        if y < min_y:
            min_y = y

    for item in collection:
        item[1] -= min_x
        item[2] -= min_y

    n_stacked = 0
    for i in range(10000000):
        for tiles, x, y in collection:
            tile_type, number = tile_to_engine_notation(tiles[0])
            print("board->tiles[%d * BOARD_SIZE + %d].type = make_tile(%s, %d);" % (y, x, tile_type, number))
            for i, stack_tile in enumerate(reversed(tiles[1:])):
                tile_type, number = tile_to_engine_notation(stack_tile)
                print("board->stack[%d].location = %d * BOARD_SIZE + %d;" % (n_stacked, y, x))
                print("board->stack[%d].z = %d;" % (n_stacked, i))
                print("board->stack[%d].type = make_tile(%s, %d);" % (n_stacked, tile_type, number))
                n_stacked += 1



if __name__ == "__main__":
    main()
