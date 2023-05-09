from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CppExtension

setup(
    name='mcts',
    ext_modules=[
        CppExtension('mcts', [
            'ml/ai_mcts.cpp',
        ])
    ],
    cmdclass={
        'build_ext': BuildExtension
    })
