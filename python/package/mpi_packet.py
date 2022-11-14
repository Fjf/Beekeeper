class MPIPacketState:
    PROCESS = 0
    TERMINATE = 1


class MPIPacket:
    def __init__(self, state: MPIPacketState, data):
        self.state = state
        self.data = data
