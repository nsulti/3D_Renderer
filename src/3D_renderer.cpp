#include <MiniFB.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
// важные константы
const int WIDTH = 1280;
const int HEIGHT = 960;
const double pi = 3.1415926535f;
const int FOV_degrees = 90;
const double FOV = FOV_degrees * pi / 180.0f;
const double HalfFOVtan = std::tan(FOV / 2.0f);
const double f_far = 1000.0f;
const double n_near = 0.01f;
const float ambient = 0.3f;
const float shiny = 128.0f;

// изменяемые глобальные переменные
// float multiplier = 0.5f; бывшие костыли, оставляю на память в честь моего позора
// float step = 0.5f; 
float aspect_ratio = (float) WIDTH / (float) HEIGHT;
int dot_wireframe_color = 0x000000;
int dot_horizon_color = 0xCFCFCF;
int shadow_color = 0xb9b9b9;
int background_color = 0xFFFFFF;
int bulb_color = 0xFFE297;
// градусы для матриц вращения
int degrees_theta = 0;
int degrees_phi = 0;

struct Vector3D {
    float x, y, z;
    float light = 0.0f;
    Vector3D operator+(const Vector3D& other) const {
      return Vector3D{x + other.x, y + other.y, z + other.z};
    }

    Vector3D operator-(const Vector3D& other) const {
      return Vector3D{x - other.x, y - other.y, z - other.z};
    }
    
    float length() const {
      return std::sqrt(x * x + y * y + z * z);
    }
    
    Vector3D cross_product(const Vector3D& other) {
      return Vector3D {
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
      };
    }

    Vector3D normalize() const {
      float len = length();
      //if (len > 0.0f) {
        return Vector3D{x / len, y / len, z / len};
      //}
      //return Vector3D{0, 0, 0};
    }

    float cosine(const Vector3D& other) const {
      return x * other.x + y * other.y + z * other.z;
    }
};


// центры объектов
Vector3D cube_center{0, 0, -16};


// камера
Vector3D lightsource{5, -12, 10};



// задаем структуру ребер
struct edge {
  Vector3D* start;
  Vector3D* end;
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
    Vector3D transform_normal(const Vector3D& v) const {
      float x_out = m[0] * v.x + m[1] * v.y + m[2] * v.z;
      float y_out = m[4] * v.x + m[5] * v.y + m[6] * v.z;
      float z_out = m[8] * v.x + m[9] * v.y + m[10] * v.z;
      return Vector3D{x_out, y_out, z_out};
    }
};

// злоебучий структ для хекса (сделано для смены освещения на грани)
struct Color {
  int r = 0, g = 0, b = 0;

  Color FromHex(int hex) {
    int r_ret = (hex >> 16) & 0xFF;
    int g_ret = (hex >> 8) & 0xFF;
    int b_ret = hex & 0xFF;
    return Color{r_ret, g_ret, b_ret};
  }
 
  Color& Saturate(float change) {
    r = (int) (r * change);
    g = (int) (g * change);
    b = (int) (b * change);
    return *this;
  }

  int ToHex() {
    return (r << 16) | (g << 8) | (b);
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
  float movement_step = 0.5f;
  if (isPressed) {
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
      case KB_KEY_I:
        lightsource.z -= movement_step;
        break;
      case KB_KEY_K:
        lightsource.z += movement_step;
        break;
      case KB_KEY_J:
        lightsource.x += movement_step;
        break;
      case KB_KEY_L:
        lightsource.x -= movement_step;
        break;
      case KB_KEY_U:
        lightsource.y -= movement_step;
        break;
      case KB_KEY_O:
        lightsource.y += movement_step;
        break;
    }
  }
}




// применим алгоритм брезенхема для отрисовки границ
void draw_line(const Vector3D* dot1, const Vector3D* dot2, std::vector<int>& buffer, int* color) {
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
      buffer[x0 + y0 * WIDTH] = *color;
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

// применим алгоритм брезенхема для анализа где закрашивать
std::vector<int> left_x(HEIGHT, 99999);
std::vector<int> right_x(HEIGHT, -99999);
std::vector<float> left_light(HEIGHT, 0);
std::vector<float> right_light(HEIGHT, 0);
void analyze_line(const Vector3D* dot1, const Vector3D* dot2) {
  int x0 = (int) ((dot1->x + 1.0f) * 0.5f * WIDTH);
  int y0 = (int) ((1.0f - dot1->y) * 0.5f * HEIGHT);

  int x1 = (int) ((dot2->x + 1.0f) * 0.5f * WIDTH);
  int y1 = (int) ((1.0f - dot2->y) * 0.5f * HEIGHT);

  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;
  
  // будем считать свет по процентам
  int y_total_steps = std::abs(y1 - y0);
  int y_count = 0;

  while (true) {
    if ((unsigned int) x0 < WIDTH && (unsigned int) y0 < HEIGHT) {
      float current_light;
      if (y_total_steps > 0) {
        float change = y_count / (float) y_total_steps;
        current_light = dot1->light + (dot2->light - dot1->light) * change;
      } else { current_light = dot1->light; }
      if (x0 < left_x[y0]) { left_x[y0] = x0; left_light[y0] = current_light; }
      if (x0 > right_x[y0]) { right_x[y0] = x0; right_light[y0] = current_light; }
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
      y_count++;
    }
  }
  
}

// не самая сложная логика закраски полигонов
// пояснение алгоритма: брезенхем на данном у отмечает начало и конец х
// идя сверху вниз он для каждого у пишет начальный х, меняя его при необходимости, и пишет конечный у
// переводим координаты в пиксели на экране
// и проходим по полученным координатам
void fill_polygon(const Vector3D* d1, const Vector3D* d2, const Vector3D* d3, const Vector3D* d4, std::vector<int>& buffer, int* color) {
  std::fill(left_x.begin(), left_x.end(), 99999);
  std::fill(right_x.begin(), right_x.end(), -99999);
  std::fill(left_light.begin(), left_light.end(), 0.0f);
  std::fill(right_light.begin(), right_light.end(), 0.0f);


  analyze_line(d1, d2);
  analyze_line(d1, d3);
  analyze_line(d4, d2);
  analyze_line(d4, d3);
  int screen_y1 = (int)((1.0f - d1->y) * 0.5f * HEIGHT);
  int screen_y2 = (int)((1.0f - d2->y) * 0.5f * HEIGHT);
  int screen_y3 = (int)((1.0f - d3->y) * 0.5f * HEIGHT);
  int screen_y4 = (int)((1.0f - d4->y) * 0.5f * HEIGHT);
  
  int raw_min_y = (int) std::min({screen_y1, screen_y2, screen_y3, screen_y4});
  int raw_max_y = (int) std::max({screen_y1, screen_y2, screen_y3, screen_y4});
  int min_y = std::max({0, raw_min_y});
  int max_y = std::min({HEIGHT - 1, raw_max_y});

  for (int y = min_y; y <= max_y; y++) {
    int x_start = left_x[y];
    int x_end = right_x[y];
    
    float light_start = left_light[y];
    float light_end = right_light[y];

    int width = x_end - x_start;
    if (width == 0) { width = 1; }
    float light_delta = (light_end - light_start) / width;
    float current_light = light_start;

    if (x_start > x_end || x_start == 99999) { continue; }

    int render_start = std::max(0, x_start);
    int render_end = std::min(WIDTH - 1, x_end);

    for (int x = render_start; x <= render_end; x++) {
      float l = std::max(0.1f, std::min(1.0f, current_light));
      Color RGB_before = Color().FromHex(*color);
      int Pixel = RGB_before.Saturate(l).ToHex();
      buffer[x + y * WIDTH] = Pixel;
      current_light += light_delta;
    }
  }
} 

// берем полигоны из квадратов
struct face{
  int v1, v2, v3, v4;
};
// эта штука берет индексы для постройки ребер





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
  

  // бэм бэм бэм составляем грани
  std::vector<face> cube_face = {
    {0, 1, 2, 3}, //?????????
    {2, 3, 6, 7},
    {1, 5, 3, 7}, // 1-2 4-5
    {4, 5, 0, 1},
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
  
  // лампочка
  int bulb_radius = 2;

  const std::vector<Vector3D>& selected = shapes[shape - 1];
  std::vector<Vector3D> changeable_shape_coordinates = selected;
  std::vector<Vector3D> shape_coordinates_no_projection = selected;
  std::vector<std::vector<face>> possible_faces = {cube_face,};
  // грани выбранной фигуры
  const std::vector<face> chosen_faces = possible_faces[shape - 1];
  // просчитываем нормали граней
  std::vector<Vector3D> normales;
  for (int i = 0; i < chosen_faces.size(); i++) {
    Vector3D A = selected[chosen_faces[i].v2] - selected[chosen_faces[i].v3];
    Vector3D B = selected[chosen_faces[i].v1] - selected[chosen_faces[i].v3];
    Vector3D C = A.cross_product(B).normalize();
    normales.push_back(C);
  }
  std::vector<Vector3D> normals_transformed = normales;
  std::vector<Vector3D> vertex_normals(selected.size(), Vector3D{0,0,0});
  for (int i = 0; i < chosen_faces.size(); i++) {
    vertex_normals[chosen_faces[i].v1] = vertex_normals[chosen_faces[i].v1] + normales[i];
    vertex_normals[chosen_faces[i].v2] = vertex_normals[chosen_faces[i].v2] + normales[i];
    vertex_normals[chosen_faces[i].v3] = vertex_normals[chosen_faces[i].v3] + normales[i];
    vertex_normals[chosen_faces[i].v4] = vertex_normals[chosen_faces[i].v4] + normales[i];
  }

  for (auto& n : vertex_normals) n = n.normalize();
  std::vector<Vector3D> vertex_normals_transformed = vertex_normals;
    

  // размерность картинки
  std::vector<int> pixel_buffer(WIDTH * HEIGHT);
  // запускаем окно
  struct mfb_window *window = mfb_open("3D RENDER", WIDTH, HEIGHT);
  mfb_set_target_fps(24);
  if (!window) {
    std::cerr << "Не получилось открыть окно (как?)" << std::endl;
    return -1;
  }
   
  mfb_set_keyboard_callback(window, readkeys);
   
  Vector3D horizon_point{0, 0, -10000};
  // основной цикл просчитывания всего
  while (true) {
    // задаем фон окна
    std::fill(pixel_buffer.begin(), pixel_buffer.end(), background_color);
    // их радианы для функций
    double phi = degrees_phi * pi / 180.0f;
    double theta = degrees_theta * pi / 180.0f;

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
    Matrix_4x4 RotationMatrix = ModelMatrix;
    for (int i = 0; i < vertex_normals.size(); i++) {
      vertex_normals_transformed[i] = RotationMatrix.transform_normal(vertex_normals[i]);
    }
    for (int i = 0; i < normales.size(); i++) {
      normals_transformed[i] = RotationMatrix.transform_normal(normales[i]);
    }
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
    // сразу добавим их в z буфер
    Vector3D lightsource_transformed = ModelMatrix * lightsource;
    int z_buffer_coordinates[changeable_shape_coordinates.size()] = {0};
    for (int i = 0; i < changeable_shape_coordinates.size(); ++i) {
      changeable_shape_coordinates[i] = TrueRotation * selected[i];
      shape_coordinates_no_projection[i] = ModelMatrix * selected[i];
      z_buffer_coordinates[i] = 1 / (float) changeable_shape_coordinates[i].z;
    }


    // на точку горизонта тоже :)
    Vector3D horizon_point_transformed = TrueRotation * horizon_point;
    for (int i = 0; i < changeable_horizon_dots.size(); i++) {
      changeable_horizon_dots[i] = TrueRotation * horizon_dots[i];
      draw_line(&changeable_horizon_dots[i], &horizon_point_transformed, pixel_buffer, &dot_horizon_color);
    }
    
    // бэм бэм бэм вычисляем горизонтальные линии горизонта
    Vector3D horizontal_line_left_transformed = TrueRotation * horizontal_line_left;
    Vector3D horizontal_line_right_transformed = TrueRotation * horizontal_line_right;
    
    Vector3D delta = Vector3D{0.0f, 0.0f, -h};

    for (float z = 2.0f; z > -25.0f; z--) { 
      // время мегаумной оптимизации! умножать очень дорогая операция. проще заранее создать, затем использовать дельту для эффективностиы
      draw_line(&horizontal_line_left_transformed, &horizontal_line_right_transformed, pixel_buffer, &dot_horizon_color);
      horizontal_line_left_transformed = TrueRotation * (horizontal_line_left + delta);
      horizontal_line_right_transformed = TrueRotation * (horizontal_line_right + delta);
      delta.z -= h;
    }
    
    // считаем тени ахаххаа
    for (int j = 0; j < shape_coordinates_no_projection.size(); j++) {
      Vector3D vertex = shape_coordinates_no_projection[j];
      Vector3D normal = vertex_normals_transformed[j];

      Vector3D raw_to_light = lightsource_transformed - shape_coordinates_no_projection[j];
      Vector3D L = raw_to_light.normalize();

      float diffuse_intensity = normal.cosine(L);
      if (diffuse_intensity < 0.0f) { diffuse_intensity = 0.0f; }
      
      Vector3D raw_to_camera = Vector3D{0,0,1} - vertex;
      Vector3D V = raw_to_camera.normalize();

      Vector3D H = Vector3D{L.x + V.x, L.y + V.y, L.z + V.z}.normalize();
      
      float spec_dot = normal.cosine(H);
      if (spec_dot < 0.0f) { spec_dot = 0.0f; }

      float specular_intensity = std::pow(spec_dot, shiny);
      
      float final_light = ambient + diffuse_intensity + specular_intensity;
      if (final_light > 1.0f) { final_light = 1.0f; }
      if (final_light < 0.0f) { final_light = 0.0f; }

      changeable_shape_coordinates[j].light = final_light;
    }

    // поскольку максимум граней может быть 3 на экране (именно для куба), мы просто просчитываем какие точки ЕСТЬ на экране заранее
    // выглядит сложно, но первая строка цикла ищет вектор от точки до камеры и нормализует его
    for (int i = 0; const auto& faces : chosen_faces) {
      Vector3D to_camera{0,0,1};
      if (to_camera.cosine(normals_transformed[i]) < 0.2f) { i++; continue; } // идеальное число для
      Vector3D* dot1 = &changeable_shape_coordinates[faces.v1]; // прятания граней
      Vector3D* dot2 = &changeable_shape_coordinates[faces.v2];
      Vector3D* dot3 = &changeable_shape_coordinates[faces.v3];
      Vector3D* dot4 = &changeable_shape_coordinates[faces.v4];
      
      // задаем освещение (косинус угла между камерой ии нормалью точки)
      
      
      // заполняем полигоны цветом передав точки
      fill_polygon(dot1, dot2, dot3, dot4, pixel_buffer, &shadow_color);

      // обводка полигона
      draw_line(dot1, dot2, pixel_buffer, &dot_wireframe_color);
      draw_line(dot1, dot3, pixel_buffer, &dot_wireframe_color);
      draw_line(dot4, dot3, pixel_buffer, &dot_wireframe_color);
      draw_line(dot4, dot2, pixel_buffer, &dot_wireframe_color);
      draw_line(&lightsource_transformed, dot1, pixel_buffer, &bulb_color);
      draw_line(&lightsource_transformed, dot3, pixel_buffer, &bulb_color);
      draw_line(&lightsource_transformed, dot2, pixel_buffer, &bulb_color);
      draw_line(&lightsource_transformed, dot4, pixel_buffer, &bulb_color);
      i++;
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

