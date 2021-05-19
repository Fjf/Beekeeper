


int mv(int* matrix, int n, int vector) {
    int result = 0;
    for (int i = 0; i < n; i++) {
        // Bitshift result with 1
        result <<= 1;
        int row = matrix[i];

        result |= ((row & vector) > 0);
    }

    return result;


    /*
     *  v = 1 0 0
     *
     *      1 0 1
     *  m = 0 0 0
     *      1 1 0
     *
     *
     *  r = 1 0 1
     */
}