import torch
from torch.utils.data import Dataset


class HiveDataset(Dataset):
    def __init__(self, boards: torch.Tensor, expected: torch.Tensor, outcomes: torch.Tensor, cuda=False):
        self.cuda = cuda
        # if cuda:
        #     device = torch.device("cuda", 0)
        # else:
        #     device = torch.device("cpu")

        # self.boards = boards.to(device)
        # self.expected = expected.to(device)
        # self.outcomes = outcomes.to(device)
        self.boards = boards
        self.expected = expected
        self.outcomes = outcomes

    def __len__(self):
        return len(self.boards)

    def __getitem__(self, idx):
        return self.boards[idx], self.expected[idx], self.outcomes[idx]

    def __add__(self, other: 'HiveDataset') -> 'HiveDataset':
        new_boards = torch.cat((self.boards, other.boards), 0)
        new_expected = torch.cat((self.expected, other.expected), 0)
        new_outcomes = torch.cat((self.outcomes, other.outcomes), 0)

        out = HiveDataset(new_boards, new_expected, new_outcomes, cuda=self.cuda)
        return out
