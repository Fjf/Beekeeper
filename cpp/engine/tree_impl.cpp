
#ifndef BEEKEEPER_HIVE_IMPL
#define BEEKEEPER_HIVE_IMPL
#include "tree.cpp"

class DefaultData {};

class MCTSData {
public:
    float value = 0.0f;
    int visitCount = 0;
};

template class BaseNode<MCTSData>;
template class BaseNode<DefaultData>;

#endif //BEEKEEPER_HIVE_IMPL