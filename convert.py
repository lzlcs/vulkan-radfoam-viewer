import numpy as np
import configargparse
import warnings

import tqdm

from plyfile import PlyData, PlyElement

warnings.filterwarnings("ignore")

import torch

from configs import *
from radfoam_model.scene import RadFoamScene


seed = 42
torch.random.manual_seed(seed)
np.random.seed(seed)

def save_sh_ply(model, ply_path):
    
    points = model.primal_points.detach().float().cpu().numpy()
    density = model.get_primal_density().detach().float().cpu().numpy()
    color_attributes = (
        model.get_primal_attributes().detach().float().cpu().numpy()
    )
    adjacency = model.point_adjacency.cpu().numpy()
    adjacency_offsets = model.point_adjacency_offsets.cpu().numpy()

    vertex_data = []
    for i in tqdm.trange(points.shape[0]):
        vertex_data.append(
            (
                points[i, 0],
                points[i, 1],
                points[i, 2],
                density[i, 0],
                adjacency_offsets[i + 1],
                *[
                    color_attributes[i, j]
                    for j in range(color_attributes.shape[1])
                ],
            )
        )

    dtype = [
        ("x", np.float32),
        ("y", np.float32),
        ("z", np.float32),
        ("density", np.float32),
        ("adjacency_offset", np.uint32),
    ]

    for i in range(color_attributes.shape[1]):
        dtype.append(("color_sh_{}".format(i), np.float32))

    vertex_data = np.array(vertex_data, dtype=dtype)
    vertex_element = PlyElement.describe(vertex_data, "vertex")

    adjacency_data = np.array(adjacency, dtype=[("adjacency", np.uint32)])
    adjacency_element = PlyElement.describe(adjacency_data, "adjacency")

    PlyData([vertex_element, adjacency_element]).write(ply_path)


def convert(args, pipeline_args, model_args, optimizer_args, dataset_args):
    
    checkpoint = args.config.replace("/config.yaml", "")
    device = torch.device(args.device)
    # Setting up model
    model = RadFoamScene(args=model_args, device=device)

    model.load_pt(f"{checkpoint}/model.pt")
    save_sh_ply(model, f"{checkpoint}/sh_scene.ply")


def main():
    parser = configargparse.ArgParser()

    model_params = ModelParams(parser)
    dataset_params = DatasetParams(parser)
    pipeline_params = PipelineParams(parser)
    optimization_params = OptimizationParams(parser)

    # Add argument to specify a custom config file
    parser.add_argument(
        "-c", "--config", is_config_file=True, help="Path to config file"
    )
    
    # Parse arguments
    args = parser.parse_args()

    convert(
        args,
        pipeline_params.extract(args),
        model_params.extract(args),
        optimization_params.extract(args),
        dataset_params.extract(args),
    )


if __name__ == "__main__":
    main()
