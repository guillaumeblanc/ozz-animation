`%>%` <- magrittr::`%>%`

LoadExperience <- function(experience, path) {
  # Transforms data
  filename <- file.path(path, paste(experience, "_transforms.csv", sep=""))
  dft <- read.csv(filename)
  dft$experience <- as.factor(experience)

  # Skeleton data
  filename <- file.path(path, paste(experience, "_skeleton.csv", sep=""))
  dfs <- read.csv(filename)

  # compression info
  filename <- file.path(path, paste(experience, "_compression.csv", sep=""))
  dfc <- read.csv(filename)
  dfc$experience <- as.factor(experience)

  # Tracks info
  filename <- file.path(path, paste(experience, "_tracks.csv", sep=""))
  dftr <- read.csv(filename)
  dftr$experience <- as.factor(experience)

  # Performance info
  filename <- file.path(path, paste(experience, "_performance.csv", sep=""))
  dfp <- read.csv(filename)
  dfp$experience <- as.factor(experience)

  return (list(skeleton=dfs, transforms=dft, compression=dfc, tracks=dftr, performance=dfp))
}

LoadExperiences <- function(path, reference) {

  # Finds and load all relevant experiences in path
  #------------------------------------------------
  experiences <- strsplit(list.files(path = path, pattern = "_compression.csv$"), "_compression.csv")
  # Moves reference first
  experiences <- c(reference, experiences[experiences != reference])
  ldf <- lapply(experiences, LoadExperience, path)

  # Check if some joints shall be rejected
  reject_filename <- file.path(params$path, "reject.csv")
  if (file.exists(reject_filename)) {
    reject <- read.csv(reject_filename)
  } else {
    reject <- data.frame(name="unknown")
  }

  # Prepare skeleton dataframe
  #---------------------------
  # Ensure all skeleton experiences are the same
  #if(!all(df1$skeleton == df2$skeleton && df1$skeleton == df3$skeleton)) {
  #  message("skeleton data are not consistent between experiences.")
  #  quit(save = "no", status=1)
  #}
  skeleton <- ldf[[1]]$skeleton

  # Prepares dataframe with transforms
  #-----------------------------------
  transforms <- dplyr::left_join(do.call(rbind, lapply(ldf, function(i) i$transforms)), skeleton, by="joint")

  #if(params$subset_data) {
  #  transforms <- subset(transforms, joint < 10 & time < .2)
  #transforms <- subset(transforms, time >= 10.80 & time <= 10.83)
  #}

  transforms$joint<- factor(transforms$joint)
  transforms$name <- factor(transforms$name)

  # Sorted by joint
  #transforms$name <- reorder(transforms$name, as.numeric(transforms$joint))
  # Sorted by depth
  transforms$name <- reorder(transforms$name, -transforms$depth)

  transforms$depth <- as.factor(transforms$depth)
  transforms$depth <- reorder(transforms$depth, -as.integer(transforms$depth))

  # Rejects unwanted joints
  transforms <- transforms[!transforms$name %in% reject$name,]
  transforms <- transforms[with(transforms, order(experience, joint, time)),]

  locals <- transforms %>%
    dplyr::select(joint, depth, name, time, experience, t.x=lt.x, t.y=lt.y, t.z=lt.z, r.x=lr.x, r.y=lr.y, r.z=lr.z, r.w=lr.w, s.x=ls.x, s.y=ls.y, s.z=ls.z)
  models <- transforms %>%
    dplyr::select(joint, depth, name, time, experience, t.x=mt.x, t.y=mt.y, t.z=mt.z, r.x=mr.x, r.y=mr.y, r.z=mr.z, r.w=mr.w, s.x=ms.x, s.y=ms.y, s.z=ms.z)

  # Computes errors
  #----------------
  locals_ref <- locals %>% dplyr::filter(experience == reference)
  models_ref <- models %>% dplyr::filter(experience == reference)

  locals$skinning_error <- ozzr::ComputeSkinningError(locals, locals_ref, params$vertex_distance)
  models$skinning_error <- ozzr::ComputeSkinningError(models, models_ref, params$vertex_distance)

  locals$t.e <- ozzr::ComputeTranslationError(locals, locals_ref)
  models$t.e <- ozzr::ComputeTranslationError(models, models_ref)

  locals$r.e <- ozzr::ComputeRotationError(locals, locals_ref, params$vertex_distance)
  models$r.e <- ozzr::ComputeRotationError(models, models_ref, params$vertex_distance)

  locals$s.e <- ozzr::ComputeScaleError(locals, locals_ref)
  models$s.e <- ozzr::ComputeScaleError(models, models_ref)

  # compression size dataframe
  #----------------------
  compression <- do.call(rbind, lapply(ldf, function(i) i$compression))

  # Tracks dataframe
  #-----------------
  tracks <- dplyr::left_join(do.call(rbind, lapply(ldf, function(i) i$tracks)), dplyr::select(skeleton, c("joint", "name")), by="joint")
  tracks <- tracks[!tracks$name %in% reject$name,]
  tracks <- tracks[with(tracks, order(joint)),]

  # Performance dataframe
  #----------------------
  performance <- do.call(rbind, lapply(ldf, function(i) i$performance))
  performance$mode <- factor(performance$mode, levels = c("forward", "forward-cache", "backward", "backward-cache", "random", "random-cache", "reset", "context"))

  # Returns
  return (list(experiences=experiences, locals=locals, models=models, compression=compression, tracks=tracks, performance=performance))
}

ComputeSkinningError <- function(df, df_ref, distance) {
  # Skinning estimation. Transform 3 orthogonal vertices for each table entry, and stores the maximum error
  vertices <- data.frame(x=c(distance, 0., 0.), y=c(0., distance, 0.), z=c(0., 0., distance))

  res <- apply(apply(vertices, 1, function(i) {
    affine_transform_point_df <- function(ldf, v) {
      t <- dplyr::select(ldf, x=t.x, y=t.y, z=t.z)
      r <- dplyr::select(ldf, x=r.x, y=r.y, z=r.z, w=r.w)
      s <- dplyr::select(ldf, x=s.x, y=s.y, z=s.z)
      affine_transform_point(t, r, s, v)
    }
    v <- data.frame(x=i[1], y=i[2], z=i[3])
    float3_distance(affine_transform_point_df(df, v), affine_transform_point_df(df_ref, v))
  }),
  1, max)

  return (res * 1000)
}

ComputeTranslationError <- function(df, df_ref) {
  res <- ozzr::float3_distance(dplyr::select(df, x=t.x, y=t.y, z=t.z),
                                dplyr::select(df_ref, x=t.x, y=t.y, z=t.z))
  return (res * 1000)
}

ComputeRotationError <- function(df, df_ref, distance) {
  vertices <- data.frame(x=c(distance, 0., 0.), y=c(0., distance, 0.), z=c(0., 0., distance))

  # Rotation error, transformation based.
  res <- apply(apply(vertices, 1, function(i) {
    quat_transform_vector_df <- function(ldf, v) {
      ozzr::quat_transform_vector(dplyr::select(ldf, x=r.x, y=r.y, z=r.z, w=r.w), v)
    }
    v <- data.frame(x=i[1], y=i[2], z=i[3])
    ozzr::float3_distance(quat_transform_vector_df(df, v),
                           quat_transform_vector_df(df_ref, v))
  }),
  1, max)

  return (res * 1000)
}

ComputeScaleError <- function(df, df_ref) {
  res <- ozzr::float3_distance(dplyr::select(df, x=s.x, y=s.y, z=s.z),
                                dplyr::select(df_ref, x=s.x, y=s.y, z=s.z))
  return (res)
}
