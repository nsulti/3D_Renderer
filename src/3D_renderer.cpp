#include <MiniFB.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>

// важные константы
const int WIDTH = 1280;
const int HEIGHT = 960;
const double pi = 3.1415926535f;
const int FOV_degrees = 90;
const double FOV = FOV_degrees * pi / 180.0f;
const double HalfFOVtan = std::tan(FOV / 2.0f);
const double f_far = 1000.0f;
const double n_near = 0.01f;

// изменяемые глобальные переменные
// float multiplier = 0.5f; бывшие костыли, оставляю на память в честь моего позора
// float step = 0.5f; 
float aspect_ratio = (float) WIDTH / (float) HEIGHT;
int dot_wireframe_color = 0x000000;
int dot_horizon_color = 0xCFCFCF;
int shadow_color = 0x6B6B6B;

// градусы для матриц вращения
int degrees_theta = 0;
int degrees_phi = 0;

// создаем переменную которая будет считать, нужно ли вообще перерисовывать кадр.
bool needs_redraw = true;

// задаем структуру вектора (по факту, является координатой тоже)
struct Vector3D {
    float x, y, z;

    Vector3D operator+(const Vector3D& other) const {
      return Vector3D{x + other.x, y + other.y, z + other.z};
    }

    Vector3D operator-(const Vector3D& other) const {
      return Vector3D{x - other.x, y - other.y, z - other.z};
    }
};


// центры объектов
Vector3D cube_center{0, 0, -14};

// задаем структуру ребер
struct edge {
  int start_index;
  int end_index;
};


// задаем структуру матрицы
struct Matrix_4x4 {
    double m[16] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1
    };
 
    Matrix_4x4 operator*(const Matrix_4x4& other) const {
      Matrix_4x4 result;
      for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
          float sum = 0.0f;
          for (int i = 0; i < 4; ++i) {
            sum += this->m[row * 4 + i] * other.m[i * 4 + col];
          }
          result.m[row * 4 + col] = sum;
        }
      }
      return result;
    }

    Vector3D operator*(const Vector3D& v) const {
      float x_out = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
      float y_out = m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7];
      float z_out = m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11];
      float w_out = m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15];

      if (w_out != 0.0f && w_out != 1.0f) {
        float invW = 1 / w_out; // считаем 1 раз для эффективности
        return Vector3D{x_out * invW, y_out * invW, z_out * invW};
      }
      return Vector3D{x_out, y_out, z_out};
    }
};

// злоебучий структ для хекса (сделано для смены освещения на грани)
struct Color {
  int r = 0, g = 0, b = 0;

  static Color FromHex(int hex) {
    int r = (hex >> 16) & 0xFF;
    int g = (hex >> 8) & 0xFF;
    int b = hex & 0xFF;
    return Color{r, g, b};
  }
 
  static Color Saturate(Color RGB, float change) {
    return Color{(int) (RGB.r * change), (int) (RGB.g * change), (int) (RGB.b * change)};
  }

  static int ToHex(Color RGB) {
    return (RGB.r << 16) | (RGB.g << 8) | (RGB.b);
  }
};

// переводим координаты в позицию пикселя
int coords_to_screen(const Vector3D* dot) {
  int screen_x = (int) ((dot->x + 1.0f) * 0.5f * WIDTH);
  int screen_y = (int) ((1.0f - dot->y) * 0.5f * HEIGHT);
  if (screen_x < 0 || screen_x >= WIDTH || screen_y < 0 || screen_y >= HEIGHT) {
    return -1;
  }
  return (int) (screen_x + screen_y * WIDTH);
}

// читаем клавиши
void readkeys(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed) {
  if (isPressed) {
    needs_redraw = true;
    switch (key) {
      case KB_KEY_UP:
        if (degrees_phi > -90) { degrees_phi -= 5; };
        break;
      case KB_KEY_DOWN:
        if (degrees_phi < 0) { degrees_phi += 5; }; // АХАХАХХАХА я здесь ограничиваю
        break;                                      // чтобы не ломалась логика горизонта XDDD
      case KB_KEY_LEFT:
        if (degrees_theta < 90) { degrees_theta += 5; };
        break;
      case KB_KEY_RIGHT:
        if (degrees_theta > -90) { degrees_theta -= 5; };
        break;
    }
  }
}


// высчитываем координату + проекцию сразу
void place_dot(const Vector3D* dot, std::vector<int>& buffer) {
  Vector3D screen_dot{dot->x, dot->y, dot->z};
  int dot_pixel = coords_to_screen(dot);
  if (dot_pixel >= 0 && dot_pixel < HEIGHT * WIDTH) { buffer[dot_pixel] = dot_wireframe_color; }
}

// применим алгоритм брезенхема
void draw_line(const Vector3D* dot1, const Vector3D* dot2, std::vector<int>& buffer, int color) {
  int x0 = (int) ((dot1->x + 1.0f) * 0.5f * WIDTH);
  int y0 = (int) ((1.0f - dot1->y) * 0.5f * HEIGHT);

  int x1 = (int) ((dot2->x + 1.0f) * 0.5f * WIDTH);
  int y1 = (int) ((1.0f - dot2->y) * 0.5f * HEIGHT);

  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  

  while (true) {

    if ((unsigned int) x0 < WIDTH && (unsigned int) y0 < HEIGHT) { // шотаем минусовые координаты через uint
      buffer[x0 + y0 * WIDTH] = color;
    }
    if (x0 == x1 && y0 == y1) { break; }

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
  
}


int main() {
  // Oy <=> WIDTH / 2
  // Ox <=> HEIGHT / 2
  // заранее просчитываем координаты фигур, чтобы не пересчитывать каждый раз их при генерации координат
  // куб с центром в (0, 0, -10)
  int cube_size = 6;
  float h = cube_size / 2; // h - половина ребра
  
  // координаты 
  Vector3D dot1{h, h, h};
  Vector3D dot2{h, -h, h};
  Vector3D dot3{-h, h, h};
  Vector3D dot4{-h, -h, h};
  Vector3D dot5{h,  h, -h};
  Vector3D dot6{h, -h, -h};
  Vector3D dot7{-h, h, -h};
  Vector3D dot8{-h, -h, -h};
  std::vector<Vector3D> cube_points = {dot1, dot2, dot3, dot4, dot5, dot6, dot7, dot8};
  std::vector<edge> cube_edges = {
    {0, 1}, {1, 3}, {2, 3}, {0, 2}, // задний квадрат
    {4, 5}, {4, 6}, {5, 7}, {6, 7}, // передний квадрат
    {0, 4}, {2, 6}, {1, 5}, {3, 7}  // ребра, соединяющие их
  };
 

  // точки горизонта
  Vector3D horizon1{2.5f * h, h, 2.0f * h}; // без вопросов
  Vector3D horizon2{4.0f * h, h, 2.0f * h}; // метод проб и ошибок
  Vector3D horizon3{-2.5f * h, h, 2.0f * h};
  Vector3D horizon4{-4.0f * h, h, 2.0f * h};
  Vector3D horizon5{-h, h, 2.0f * h};
  Vector3D horizon6{h, h, 2.0f * h};
  // а вот тута будут точки параллельные ребру куба (почему нет?)
  Vector3D horizontal_line_left{4.0f * h, h, 2.0f * h};
  Vector3D horizontal_line_right{-4.0f * h, h, 2.0f * h};

  std::vector<Vector3D> horizon_dots{
    horizon1, horizon2, horizon3, 
    horizon4, horizon5, horizon6, 
  };
  // я ненавижу себя за этот код

  std::vector<Vector3D> changeable_horizon_dots = horizon_dots;

  // здесь когда-нибудь будет сфера
  std::vector<Vector3D> sphere_points;
  
  int sphere_radius = 0;

  std::vector<std::vector<Vector3D>> shapes = {cube_points, sphere_points};
  std::vector<int> shape_size = {cube_size, sphere_radius};
  int inv_z[WIDTH * HEIGHT] = {0}; // задаем изначальное значение z буферу
 
  // выбор фигуры (банальности)
  int shape;
  std::cout << "Отображение теней!\n1. Куб\n2. Сфера\n";
  while (true) {
    std::cout << "Введите фигуру: ";
    std::cin >> shape;
    if (shape > 3 || shape < 0) {
      std::cout << "Ошибка ввода!\n\n";
      continue;
    }
    break;
  }
  
  

  const std::vector<Vector3D>& selected = shapes[shape - 1];
  std::vector<Vector3D> changeable_shape_coordinates = selected;
  // проверяем, какие ребра можно рисовать
  std::vector<std::vector<edge>> figure_edges = {cube_edges,}; //потом добавить сферу или еще что-то 

  // ребра выбранной фигуры
  const std::vector<edge>& selected_edges = figure_edges[shape - 1];
  
  // размерность картинки
  std::vector<int> pixel_buffer(WIDTH * HEIGHT);
  // запускаем окно
  struct mfb_window *window = mfb_open("3D RENDER", WIDTH, HEIGHT);
  mfb_set_target_fps(24);
  if (!window) {
    std::cerr << "Не получилось открыть окно (как?)" << std::endl;
    return -1;
  }
  int background_color = 0xFFFFFF; 
  mfb_set_keyboard_callback(window, readkeys);
   
  Vector3D horizon_point{0, 0, -10000};
  // основной цикл просчитывания всего
  while (true) {
    // задаем фон окна
    memset(pixel_buffer.data(), background_color, sizeof(int) * WIDTH * HEIGHT);

    // их радианы для функций
    double phi = degrees_phi * pi / 180.0f;
    double theta = degrees_theta * pi / 180.0f;

    Vector3D lightsource{0, 20, 0};
    // задаем вектор света

    double sin_theta = std::sin(theta); double cos_theta = std::cos(theta); 
    double sin_phi = std::sin(phi); double cos_phi = std::cos(phi);
    // просчитываем заранее фи и тету (их косинус и синус)
    // а вот и пригодился struct матрицы. теперь код гарантированно красив и чист
    // по факту, у нас матрица должна быть PVM
    // Projection (немного головная боль)
    // View (поскольку у нас камера в 0,0,0, она становится единичной)
    // Model матрица 4х4, где 3х3 слева сверху есть матрица поворота, 4 столбец это (dx, dy, dx, w)
    // где координата d есть центр фигуры
    
    Matrix_4x4 RotYMatrix = {
      cos_theta,   0,   sin_theta,    0,
      0,           1,   0,            0,
      -sin_theta,  0,   cos_theta,    0,
      0,           0,   0,            1
    };
    Matrix_4x4 RotXMatrix = {
      1, 0,       0,        0,
      0, cos_phi, -sin_phi, 0,
      0, sin_phi, cos_phi,  0,
      0, 0,       0,        1
    };
    Matrix_4x4 ModelMatrix = RotXMatrix * RotYMatrix;
    // тут мы задаем последнему столбцу на строках 1-3 координаты куба
    // костыль? возможно. жаль что мне похуй
    ModelMatrix.m[3] = cube_center.x; ModelMatrix.m[7] = cube_center.y; ModelMatrix.m[11] = cube_center.z;
    Matrix_4x4 ProjectionMatrix = {
      1 / (aspect_ratio * HalfFOVtan),   0,                0,                                   0,
      0,                                 1 / (HalfFOVtan), 0,                                   0,
      0,                                 0,                (f_far + n_near) / (n_near - f_far), (2 * f_far * n_near) / (n_near - f_far),
      0,                                 0,                1,                                   0
    };
    
    Matrix_4x4 TrueRotation = ProjectionMatrix * ModelMatrix;
    // на каждую исходную координату применяем полученную матрицу (большую)
    for (int i = 0; i < changeable_shape_coordinates.size(); ++i) {
      changeable_shape_coordinates[i] = TrueRotation * selected[i];
    }
    // на точку горизонта тоже :)
    Vector3D horizon_point_transformed = TrueRotation * horizon_point;
    for (int i = 0; i < changeable_horizon_dots.size(); i++) {
      changeable_horizon_dots[i] = TrueRotation * horizon_dots[i];
      draw_line(&changeable_horizon_dots[i], &horizon_point_transformed, pixel_buffer, dot_horizon_color);
    }
    
    // бэм бэм бэм вычисляем горизонтальные линии горизонта
    Vector3D horizontal_line_left_transformed = TrueRotation * horizontal_line_left;
    Vector3D horizontal_line_right_transformed = TrueRotation * horizontal_line_right;
    
    Vector3D delta = Vector3D{0.0f, 0.0f, -h};

    for (float z = 2.0f; z > -25.0f; z--) { 
      // время мегаумной оптимизации! умножать очень дорогая операция. проще заранее создать, затем использовать дельту для эффективности
      draw_line(&horizontal_line_left_transformed, &horizontal_line_right_transformed, pixel_buffer, dot_horizon_color);
      horizontal_line_left_transformed = TrueRotation * (horizontal_line_left + delta);
      horizontal_line_right_transformed = TrueRotation * (horizontal_line_right + delta);
      delta.z -= h;
    }

    // выводим линии, получается wireframe фигура
    for (const auto& edges : selected_edges) {
      const Vector3D* dot1 = &changeable_shape_coordinates[edges.start_index];
      const Vector3D* dot2 = &changeable_shape_coordinates[edges.end_index];
      draw_line(dot1, dot2, pixel_buffer, dot_wireframe_color);
    }
    
    // здесь мы просчитываем и рисуем линии горизонта
    
    // :)))))))

    // не трогать; вывод буфера на экран
    int state = mfb_update_ex(window, pixel_buffer.data(), WIDTH, HEIGHT);
    if (state < 0) {
      break;
    }
    if (!mfb_wait_sync(window)) {
        break; 
    }
  }
  mfb_close(window);
  return 0;
}

