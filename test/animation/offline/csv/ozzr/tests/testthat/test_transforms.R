library(testthat)

context("cozzr transforms")

test_that("float3_distance", {
  a <- data.frame(x = 1., y = 2., z = 3.)
  d <- float3_distance(a, a)
  expect_equal(d, c(0.))

  b <- data.frame(x = c(1., -1.),
                  y = c(2.,-2.),
                  z = c(3., -3.))
  d <- float3_distance(a, b)
  expect_equal(d, c(0., 7.4833147735478827711674974646331))
})

test_that("quat_distance", {
  a <- data.frame(x = 0., y = 0., z = 0., w = 1.)
  b <- data.frame(x = 0., y = 0.70710678118654752440084436210485, z = 0., w = 0.70710678118654752440084436210485)
  r <- quat_distance(a, a)
  expect_equal(r, c(0.))

  r <- quat_distance(a, b)
  expect_equal(r, c(pi / 2.))

  r <- quat_distance(a, -b)
  expect_equal(r, c(pi / 2.))

  # Unormalized quaternions still returns NA.
  c <- data.frame(x = .467823, y = .527472, z = .530318, w = -0.470827)
  d <- data.frame(x = -.467810, y = -.527454, z = -.530363, w = 0.470809)
  r <- quat_distance(c, d)
  expect_equal(r, c(NaN))

  # Unormalized quaternions still returns 0.
  r <- quat_distance(a, 2*a)
  expect_equal(r, c(NaN))

  # Works with vectors as well
  da <- rbind(a, c)
  db <- rbind(b, d)
  r <- quat_distance(da, db)
  expect_equal(r, c(pi / 2., NaN))
})


test_that("quat_transform_vector", {
  q <- data.frame(x = 0., y = 0., z = 0., w = 1.)
  v <- data.frame(x = 1., y = 2., z = 3.)
  qv <- quat_transform_vector(q, v)
  expect_equal(qv, v)

  q <- data.frame(x = 0., y = 0.70710678118654752440084436210485, z = 0., w = 0.70710678118654752440084436210485)
  v <- data.frame(x = 1., y = 2., z = 3.)
  qv <- quat_transform_vector(q, v)
  expect_equal(qv, data.frame(x = 3., y = 2., z = -1.))
})

test_that("affine_transform_point", {
  t <- data.frame(x = 0., y = 0., z = 0.)
  q <- data.frame(x = 0.70710678118654752440084436210485, y = 0., z = 0., w = 0.70710678118654752440084436210485)
  s <- data.frame(x = 1., y = 1., z = 1.)
  p <- data.frame(x = 4., y = 5., z = 6.)

  tqsp <- affine_transform_point(t, q, s, p)
  expect_equal(tqsp ,data.frame(x = 4., y = -6., z = 5.))

  s <- data.frame(x = 10., y = 100., z = 1000.)
  tqsp <- affine_transform_point(t, q, s, p)
  expect_equal(tqsp ,data.frame(x = 40., y = -6000., z = 500.))

  t <- data.frame(x = 1., y = 2., z = 3.)
  tqsp <- affine_transform_point(t, q, s, p)
  expect_equal(tqsp ,data.frame(x = 41., y = -5998., z = 503.))
})
