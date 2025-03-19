import numpy as np
from PIL import Image
import configargparse
import warnings
 
warnings.filterwarnings("ignore")
 
import torch
import os
from torch import nn
from plyfile import PlyData, PlyElement
 
import radfoam
from radfoam_model.utils import *
from data_loader import DataHandler
from configs import *
from radfoam_model.scene import RadFoamScene
from radfoam_model.utils import psnr
import radfoam
 
 
seed = 42
torch.random.manual_seed(seed)
np.random.seed(seed)
 
 
def load_ply(model, ply_path):
    ply_data = PlyData.read(ply_path)
    
    vertex_data = ply_data["vertex"].data
    num_points = len(vertex_data)
 
    points = np.empty((num_points, 3), dtype=np.float32)
    points[:, 0] = vertex_data["x"]
    points[:, 1] = vertex_data["y"]
    points[:, 2] = vertex_data["z"]
    model.primal_points = nn.Parameter(torch.from_numpy(points).float().to(model.device))
 
    density = np.empty((num_points, 1), dtype=np.float32)
    density[:, 0] = vertex_data["density"]
    model.density = nn.Parameter(torch.from_numpy(density).float().to(model.device))
 
    adjacency_offsets = vertex_data["adjacency_offset"].astype(np.uint32)
    adjacency_offsets = np.concatenate([[0], adjacency_offsets]).astype(np.uint32)
    model.point_adjacency_offsets = torch.from_numpy(adjacency_offsets).to(model.device)
 
    sh_keys = [k for k in vertex_data.dtype.names if k.startswith("color_sh_")]
    num_sh = len(sh_keys)
    if num_sh > 0:
        sh_attributes = np.empty((num_points, num_sh), dtype=np.float32)
        for i in range(num_sh):
            sh_attributes[:, i] = vertex_data[f"color_sh_{i}"]
 
    model.att_sh = nn.Parameter(torch.from_numpy(sh_attributes[:, 3:]).float().to(model.attr_dtype).to(model.device))
    model.att_dc = nn.Parameter(torch.from_numpy(sh_attributes[:, 0:3]).float().to(model.attr_dtype).to(model.device))
 
    adjacency_data = ply_data["adjacency"].data
    adjacency = adjacency_data["adjacency"].astype(np.uint32)
    model.point_adjacency = torch.from_numpy(adjacency).to(model.device)
 
def test(args, pipeline_args, model_args, optimizer_args, dataset_args):
    checkpoint = args.config.replace("/config.yaml", "")
    os.makedirs(os.path.join(checkpoint, "test_sh_ply"), exist_ok=True)
    device = torch.device(args.device)
 
    test_data_handler = DataHandler(
        dataset_args, rays_per_batch=0, device=device
    )
    test_data_handler.reload(
        split="test", downsample=min(dataset_args.downsample)
    )
    test_ray_batch_fetcher = radfoam.BatchFetcher(
        test_data_handler.rays, batch_size=1, shuffle=False
    )
    test_rgb_batch_fetcher = radfoam.BatchFetcher(
        test_data_handler.rgbs, batch_size=1, shuffle=False
    )
 
    # Setting up model
    model = RadFoamScene(args=model_args, device=device)
 
    # model.load_pt(f"{checkpoint}/model.pt")
    load_ply(model, f"{checkpoint}/sh_scene.ply")
 
    def test_render(
        test_data_handler, ray_batch_fetcher, rgb_batch_fetcher
    ):
        rays = test_data_handler.rays
        points, _, _, _ = model.get_trace_data()
        start_points = model.get_starting_point(
            rays[:, 0, 0].cuda(), points, model.aabb_tree
        )
 
        psnr_list = []
        with torch.no_grad():
            for i in range(rays.shape[0]):
                ray_batch = ray_batch_fetcher.next()[0]
                rgb_batch = rgb_batch_fetcher.next()[0]
                output, _, _, _, _ = model(ray_batch, start_points[i])
 
                # White background
                opacity = output[..., -1:]
                rgb_output = output[..., :3] + (1 - opacity)
                rgb_output = rgb_output.reshape(*rgb_batch.shape).clip(0, 1)
 
                img_psnr = psnr(rgb_output, rgb_batch).mean()
                psnr_list.append(img_psnr)
                torch.cuda.synchronize()
 
                error = np.uint8((rgb_output - rgb_batch).cpu().abs() * 255)
                rgb_output = np.uint8(rgb_output.cpu() * 255)
                rgb_batch = np.uint8(rgb_batch.cpu() * 255)
 
                im = Image.fromarray(
                    np.concatenate([rgb_output, rgb_batch, error], axis=1)
                )
                im.save(
                    f"{checkpoint}/test_sh_ply/rgb_{i:03d}_psnr_{img_psnr:.3f}.png"
                )
 
        average_psnr = sum(psnr_list) / len(psnr_list)
 
        f = open(f"{checkpoint}/metrics.txt", "w")
        f.write(f"Average PSNR: {average_psnr}")
        f.close()
 
        return average_psnr
 
    test_render(
        test_data_handler, test_ray_batch_fetcher, test_rgb_batch_fetcher
    )
 
 
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
 
    test(
        args,
        pipeline_params.extract(args),
        model_params.extract(args),
        optimization_params.extract(args),
        dataset_params.extract(args),
    )
 
 
if __name__ == "__main__":
    main()