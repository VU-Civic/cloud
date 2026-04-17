#ifndef __MATH_UTILITIES_HEADER_H__
#define __MATH_UTILITIES_HEADER_H__

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>

#ifndef _WIN32
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

// Custom IEEE754 floating point class in case --ffinite-math-only is specified
template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value>::type, typename = typename std::enable_if<std::numeric_limits<T>::is_iec559>::type,
          typename u_t = typename std::conditional<std::is_same<T, float>::value, uint32_t, uint64_t>::type>
struct IEEE754
{
  enum class WIDTH : size_t
  {
    SIGN = 1,
    EXPONENT = std::is_same<T, float>::value ? 8 : 11,
    MANTISSA = std::is_same<T, float>::value ? 23 : 52
  };
  enum class MASK : u_t
  {
    SIGN = (u_t)1 << (sizeof(u_t) * 8 - 1),
    EXPONENT = ((~(u_t)0) << (size_t)WIDTH::MANTISSA) ^ (u_t)MASK::SIGN,
    MANTISSA = (~(u_t)0) >> ((size_t)WIDTH::SIGN + (size_t)WIDTH::EXPONENT)
  };
  union
  {
    T f;
    u_t u;
  };
  explicit IEEE754(T f) : f(f) {};
  inline u_t sign(void) const { return u & (u_t)MASK::SIGN >> ((size_t)WIDTH::EXPONENT + (size_t)WIDTH::MANTISSA); };
  inline u_t exponent(void) const { return u & (u_t)MASK::EXPONENT >> (size_t)WIDTH::MANTISSA; };
  inline u_t mantissa(void) const { return u & (u_t)MASK::MANTISSA; };
  inline bool isNan() const { return (mantissa() != 0) && ((u & ((u_t)MASK::EXPONENT)) == (u_t)MASK::EXPONENT); };
  inline bool isInf() const { return (mantissa() == 0) && ((u & ((u_t)MASK::EXPONENT)) == (u_t)MASK::EXPONENT); };
};

namespace Math
{
  // Mathematical constants
  constexpr const float PI = 3.14159265358979323846f;
  constexpr const float PI_HALF = PI / 2.0f;
  constexpr const float PI_QUARTER = PI / 4.0f;
  constexpr const float TWO_PI = PI * 2.0f;
  constexpr const float EULERS_NUMBER = 2.7182818284590452353602874713527f;
  constexpr const float E = EULERS_NUMBER;
  constexpr const float GOLDEN_RATIO = 1.6180339887498948482f;
  constexpr const float PYTHAGORAS_CONSTANT = 1.41421356237309504880168872420969807f;
  constexpr const float SQRT_OF_2 = PYTHAGORAS_CONSTANT;
  constexpr const float THEODORUS_CONSTANT = 1.73205080756887729352744634150587236f;
  constexpr const float SQRT_OF_3 = THEODORUS_CONSTANT;
  constexpr const float RADIANS_TO_DEGREES = 180.0f / PI;
  constexpr const float DEGREES_TO_RADIANS = PI / 180.0f;
  constexpr const int SPEED_OF_LIGHT_METERS_PER_SECOND = 299792458;
  static float SPEED_OF_SOUND_IN_AIR_METERS_PER_SECOND(float temperatureInCelsius = 20.0f) { return 20.05f * std::sqrt(temperatureInCelsius + 273.15f); };

  // Earth constants
  constexpr const float EARTH_EQUATORIAL_RADIUS = 6378137.0f;
  constexpr const float EARTH_ECCENTRICITY_2 = 0.006694379990141317f;

  // Divide and found functions
  template <typename T>
  inline constexpr static typename std::enable_if<(std::is_integral<T>::value), T>::type divideAndRound(T val1, T val2)
  {
    return static_cast<T>((static_cast<double>(val1) / static_cast<double>(val2)) + 0.5);
  };
  template <typename T>
  inline constexpr static typename std::enable_if<(std::is_integral<T>::value), T>::type divideAndCeiling(T val1, T val2)
  {
    return (val1 / val2) + ((val1 % val2 != 0) ? 1 : 0);
  };
  template <typename T>
  inline constexpr static typename std::enable_if<(std::is_integral<T>::value), T>::type divideAndFloor(T val1, T val2)
  {
    return static_cast<T>(static_cast<double>(val1) / static_cast<double>(val2));
  };

  // Norm functions
  template <typename T>
  inline static T norm(size_t numValues, T* values)
  {
    T norm = 0.0;
    for (size_t i = 0; i < numValues; ++i) norm += (values[i] * values[i]);
    return std::sqrt(norm);
  };
  template <typename T>
  inline static T infinityNorm(size_t numValues, T* values)
  {
    T infNorm = 0.0;
    for (size_t i = 0; i < numValues; ++i) infNorm = std::max(infNorm, std::abs(values[i]));
    return infNorm;
  };

  // NaN and Inf creation and testing functions
  inline constexpr static double nan(void) { return std::numeric_limits<double>::quiet_NaN(); };
  inline constexpr static float nanf(void) { return std::numeric_limits<float>::quiet_NaN(); };
  inline constexpr static double inf(void) { return std::numeric_limits<double>::infinity(); };
  inline constexpr static float inff(void) { return std::numeric_limits<float>::infinity(); };
  template <typename T>
  inline constexpr static bool isNan(T val)
  {
    return IEEE754<T>(val).isNan();
  };
  template <typename T>
  inline constexpr static bool isInf(T val)
  {
    return IEEE754<T>(val).isInf();
  };
};  // namespace Math

#endif  // #ifndef __MATH_UTILITIES_HEADER_H__
