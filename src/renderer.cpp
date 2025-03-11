#include "renderer.hpp"
#include "radfoam.hpp"

Renderer::Renderer(std::shared_ptr<RadFoam> pModel, std::shared_ptr<AABBTree> pAABB)
    : pModel(pModel), pAABB(pAABB)
{

}


void Renderer::render()
{
    handleInput();
    updateUniform();
}

void Renderer::handleInput()
{

}

void Renderer::updateUniform()
{
    UniformBuffer data;
    data.cameraPosition = glm::vec3(-3.1629207134246826, -0.6483269333839417, -0.17025022208690643);
    data.startPoint = pAABB->nearestNeighbor(data.cameraPosition);

    // std::cout << "start point: " << data.startPoint << std::endl;
}