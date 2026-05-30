#pragma once

struct Throttle final {
    double value;
};

struct Brakes final {
    double value;
};

struct Angle final {
    double xValue;
    // пока не используется double yValue;
    // пока не используется double zValue;
};

class Vehicle {
   public:
    Throttle getThrottle() const { return mThrottle; };
    void setThrottle(const Throttle& throttle) { mThrottle = throttle; };

    Brakes getBrakes() const { return mBrakes; };
    void setBrakes(const Brakes& brakes) { mBrakes = brakes; };

    Angle getAngle() const { return mAngle; };
    void setAngle(const Angle& angle) { mAngle = angle; }

   private:
    Throttle mThrottle;
    Brakes mBrakes;
    Angle mAngle;
};
