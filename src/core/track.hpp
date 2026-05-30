#pragma once

struct Spline final {
    // TODO:
    // продумать внутреннее устройство и внешний интерфейс
};

class Track {
   public:
    Track(double lengthKm, int difficultyLevel /*TODO: add enum or something else */) {};

    // TODO: подумать есть ли в этом коде необходимость
    // возвращает (предположительно полигональный) объект трассы
    // Mesh getMesh() const;
    // отдать часть меша для ленивой/часичной загрузки
    // Mesh getMesh(double left, double right) const;

    // возвращает сплайн трассы соответствующей сложности и длины
    Spline getSpline() const {
        return {};
    }
};
