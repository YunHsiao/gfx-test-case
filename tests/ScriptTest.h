#pragma once

#include "TestBase.h"
#include "base/threading/ConditionVariable.h"

namespace cc {

class ScriptTest : public TestBaseI {
public:
    DEFINE_CREATE_METHOD(ScriptTest)
    ScriptTest(const WindowInfo &info) : TestBaseI(info){};
    ~ScriptTest() = default;

    bool onInit() override;
    void onTick() override;
    void onDestroy() override;
    void renderThreadTick();

private:
    bool _shouldStop{false};
    ConditionVariable _cv;
};

} // namespace cc
