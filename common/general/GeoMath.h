#ifndef __GEO_MATH_HEADER_H__
#define __GEO_MATH_HEADER_H__

class GeoMath final
{
public:

  static float calculateDistance2D(float lat1, float lon1, float lat2, float lon2);
  static float calculateDistance3D(float lat1, float lon1, float ht1, float lat2, float lon2, float h2);

private:
};

#endif  // #ifndef __GEO_MATH_HEADER_H__
