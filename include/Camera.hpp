#pragma once

namespace sgfx
{
    enum class Keys : uint8_t
    {
        W,
        A,
        S,
        D,
        AUp,
        ALeft,
        ADown,
        ARight,
        TotalKeys
    };

    class Camera
    {
      public:
        void handleInput(const Keys key, const bool isKeyDown);
        void update(const float deltaTime);

        math::XMMATRIX getLookAtMatrix();

      public:
        // Note that the default values for the vectors must be set correctly (especially the W component).
        math::XMFLOAT4 m_cameraPosition{0.0f, 0.0f, -5.0f, 1.0f};

        math::XMFLOAT4 m_cameraRight{1.0f, 0.0f, 0.0f, 0.0f};
        math::XMFLOAT4 m_cameraUp{0.0f, 1.0f, 0.0f, 0.0f};
        math::XMFLOAT4 m_cameraForward{0.0f, 0.0f, 1.0f, 0.0f};

        float m_pitch{};
        float m_yaw{};

        float m_movementSpeed{20.0f};
        float m_rotationSpeed{1.0f};

        std::array<bool, enumClassValue(Keys::TotalKeys)> m_keys{};
    };
}