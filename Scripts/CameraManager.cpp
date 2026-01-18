#include "Engine.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/Renderer.hpp"
#include "Utility/EngineDefines.hpp"
#include "Window/Window.hpp"

class CameraManager final : public Script {
public:
    SCRIPT_CONSTRUCTOR;

    void Init() override;
    void Update() override;
    void CleanUp() override;
private:
    bool isHidden{};

    Camera* camera{};

    float speed = 0.25;
};

void CameraManager::Init() {
    GetRenderer().SetCurrentCamera(Camera::New(GetWindow().GetExtent(), 90, 0.1, 1000));
    camera = GetRenderer().GetCurrentCamera().get();

    camera->CalcProjectionMtx();

    camera->SetFov(90); 
    camera->SetFar(800);
    camera->SetNear(1);

    camera->position = glm::vec3(0, 1, 0);
    camera->rotation = glm::vec3(0, 0, 0);
    camera->scale = glm::vec3(1);
}

void CameraManager::Update() {
    const auto& window = GetWindow();

    const glm::vec2 mouseOffset = window.GetMouseOffset();

    if (isHidden) camera->AddRotation(glm::vec3(-mouseOffset.y / 10.f, mouseOffset.x / 10.f, 0));

    if (camera->GetRotation().x > 70.f)
        camera->SetRotation(glm::vec3(70, 0, 0));

    if (camera->GetRotation().x < -70.f)
        camera->SetRotation(glm::vec3(-70, 0, 0));

    const glm::vec3 camDirection = camera->CalcDirection();
    const glm::vec3 camDirectionR = glm::normalize(glm::cross(camDirection, glm::vec3(0, 1, 0)));

    const bool W = window.IsKeyHeld(Key::eW);
    const bool S = window.IsKeyHeld(Key::eS);
    const bool D = window.IsKeyHeld(Key::eD);
    const bool A = window.IsKeyHeld(Key::eA);
    const bool lShift = window.IsKeyHeld(Key::eLeftShift);
    const bool lCtrl = window.IsKeyHeld(Key::eLeftControl);

    const bool Q = window.IsKeyPressed(Key::eQ);
    const bool E = window.IsKeyPressed(Key::eE);
    const bool R = window.IsKeyPressed(Key::eR);

    if (W) camera->AddPosition(camDirection * speed);
    if (S) camera->AddPosition(-camDirection * speed);
    if (D) camera->AddPosition(camDirectionR * speed);
    if (A) camera->AddPosition(-camDirectionR * speed);
    if (lShift) camera->AddPosition(glm::vec3(0, speed, 0));
    if (lCtrl) camera->AddPosition(glm::vec3(0, -speed, 0));

    if (Q) {
        if (isHidden) {
            window.HideCursor(false);
            isHidden = false;
        }
        else {
            window.HideCursor(true);
            isHidden = true;
        }
    }
    if (E) {
        speed *= 1.5;
    }
    if (R) {
        speed *= 0.5;
    }
}

void CameraManager::CleanUp() {}

SCRIPT_LOADER(CameraManager)
