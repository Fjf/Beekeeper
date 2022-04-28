import os
import time

import torch
from openvino.runtime import Core

from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from games.utils import Perspectives


def convert_onnx(pt_path, dummy_input, onnx_path):
    # Initialize model
    nn = Connect4NN()
    nn.load_state_dict(torch.load(pt_path))

    # Export to onnx format
    torch.onnx.export(
        nn,
        dummy_input,
        onnx_path,
        opset_version=12
    )


def convert_vino(onnx_path, dummy_input, out_dir):
    # Construct the command for Model Optimizer
    mo_command = f"""mo
                     --input_model "{onnx_path}"
                     --input_shape "[{",".join(str(x) for x in dummy_input.shape)}]"
                     --data_type FP16
                     --output_dir "{out_dir}"
                     """
    mo_command = " ".join(mo_command.split())
    print("Model Optimizer command to convert the ONNX model to OpenVINO:")
    print(mo_command)

    # Run Model Optimizer if the IR model file does not exist
    print("Exporting TensorFlow model to IR... This may take a few minutes.")
    a = os.system(mo_command)
    print(a)


def profile_forward(pt_path, vino_path, dummy_input, samples=1000):
    # Initialize Pytorch model
    nn = Connect4NN()
    nn.load_state_dict(torch.load(pt_path))
    compiled_nn = torch.jit.script(nn)

    # Load the network in Inference Engine
    ie = Core()
    model_ir = ie.read_model(model=vino_path)
    compiled_model_ir = ie.compile_model(model=model_ir, device_name="CPU")

    # Get input and output layers
    input_layer_ir = next(iter(compiled_model_ir.inputs))
    output_layer_ir = next(iter(compiled_model_ir.outputs))

    # Run profiling Pytorch
    start = time.perf_counter()
    for i in range(samples):
        nn(dummy_input)
    print(f"PT took: {time.perf_counter() - start}")

    start = time.perf_counter()
    for i in range(samples):
        compiled_nn(dummy_input)
    print(f"Compiled PT took: {time.perf_counter() - start}")

    # Run profiling OpenVINO
    start = time.perf_counter()
    for i in range(samples):
        compiled_model_ir([dummy_input])[output_layer_ir]
    print(f"OpenVINO took: {time.perf_counter() - start}")


def main():
    out_dir = "models"
    vino_path = os.path.join(out_dir, "c4_model.xml")
    onnx_path = os.path.join(out_dir, "c4_model.onnx")
    pt_path = os.path.join(out_dir, "c4_model.pt")

    # Get dummy input format
    game = Connect4()
    dummy_input = torch.Tensor(game.node.to_np(Perspectives.PLAYER1))

    # Add batch to input format
    dummy_input = dummy_input.reshape(1, *dummy_input.shape)

    convert_onnx(pt_path, dummy_input, onnx_path)
    convert_vino(onnx_path, dummy_input, out_dir)

    profile_forward(pt_path, vino_path, dummy_input, samples=10000)


if __name__ == "__main__":
    main()
