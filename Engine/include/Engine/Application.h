//
// Created by Ilya Valentsiukevich on 22/07/2026.
//

#pragma once

class Application
{
public:
    Application();
    ~Application();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    bool m_Running = false;
};
