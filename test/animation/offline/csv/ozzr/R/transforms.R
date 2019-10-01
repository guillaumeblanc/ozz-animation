# Vector and quaternion operations

float3_distance <- function(a, b) {
  x <- a$x - b$x
  y <- a$y - b$y
  z <- a$z - b$z
  sqrt(x * x + y * y + z * z)
}

quat_distance <- function(a, b) {
  cos_half_angle <- a$x * b$x + a$y * b$y + a$z * b$z + a$w * b$w
  angle <- 2. * acos(abs(cos_half_angle))
  return (angle)
}

quat_transform_vector <- function(q, v) {
  # http://www.neil.dantam.name/note/dantam-quaternion.pdf
  # _v + 2.f * cross(_q.xyz, cross(_q.xyz, _v) + _q.w * _v);
  ax <- q$y * v$z - q$z * v$y + v$x * q$w
  ay <- q$z * v$x - q$x * v$z + v$y * q$w
  az <- q$x * v$y - q$y * v$x + v$z * q$w

  bx <- q$y * az - q$z * ay
  by <- q$z * ax - q$x * az
  bz <- q$x * ay - q$y * ax

  r <-
    data.frame(x = v$x + 2. * bx,
               y = v$y + 2. * by,
               z = v$z + 2. * bz)

  return (r)
}

affine_transform_point <- function(t, q, s, p) {
  qsv <- quat_transform_vector(q, data.frame(x=p$x * s$x, y=p$y * s$y, z=p$z * s$z))
  r <- data.frame(x= qsv$x + t$x, y=qsv$y + t$y, z=qsv$z + t$z)
  return (r)
}

