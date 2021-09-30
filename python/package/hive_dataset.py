from torch.utils.data import Dataset


class HiveDataset(Dataset):
    def __init__(self, boards, expected):
        self.boards = boards
        self.expected = expected

    def __len__(self):
        return len(self.boards)

    def __getitem__(self, idx):
        return self.boards[idx], self.expected[idx]

    def __add__(self, other: 'HiveDataset') -> 'HiveDataset':
        out = HiveDataset(self.boards + other.boards, self.expected + other.expected)
        return out
