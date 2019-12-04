# ozz report prlotting helpers

`%>%` <- magrittr::`%>%`

plot_animations <- function(models, animate) {
  p_xy <- models %>%
    ggplot2::ggplot() +
    ggplot2::geom_point(ggplot2::aes(x = t.x, y = t.y, color=skinning_error)) +
    ggplot2::scale_colour_gradient(low = "green", high = "red") +
    ggplot2::facet_wrap(ggplot2::vars(experience), nrow=1) +
    ggplot2::ggtitle("Model-space xy translations for each experience") +
    ggplot2::labs(title = "Time {frame_time}", color="Skinning error (mm)") +
    ggplot2::xlab("Model-space x") +
    ggplot2::ylab("Model-space y") +
    gganimate::transition_time(time) +
    gganimate::shadow_null()
  if(animate) {
    duration <- as.double(max(models$time))
    nframes <- as.integer(duration * 30)
    renderer = gganimate::gifski_renderer()
    gganimate::animate(p_xy, renderer = renderer, nframes=nframes, duration=duration)
  } else {
    plot(p_xy)
  }
}

plot_error_density <- function(df, col, xlabel = "Error value") {
  p1 <- df %>%
    ggplot2::ggplot() +
    ggplot2::geom_density(ggplot2::aes(x=col, color=experience, fill=experience), alpha = .5) +
    ggplot2::labs(color = "Experience name", fill = "Experience name") +
    ggplot2::xlab(NULL) +
    ggplot2::ylab("Density") +
    ggplot2::scale_fill_manual(values = palette_experiences) +
    ggplot2::scale_colour_manual(values = palette_experiences)

  p2 <- df %>%
    ggplot2::ggplot() +
    ggplot2::geom_boxplot(ggplot2::aes(x=experience, y=col, color=experience, fill=experience), alpha = .5) +
    ggplot2::scale_colour_manual(values = palette_experiences) +
    ggplot2::scale_fill_manual(values = palette_experiences) +
    ggplot2::labs(color = "Experience name", fill = "Experience name") +
    ggplot2::xlab("Experience name") +
    ggplot2::ylab(xlabel) +
    ggplot2::coord_flip()

  grid::grid.newpage()
  grid::grid.draw(rbind(ggplot2::ggplotGrob(p1), ggplot2::ggplotGrob(p2), size = "last"))
}

plot_error_per_joint <- function(df, col, xlabel = "Error value") {
  p <- df %>%
    ggplot2::ggplot(ggplot2::aes(x=name, y=col, color=experience)) +
    ggplot2::geom_boxplot() +
    ggplot2::labs(color = "Experience name") +
    ggplot2::xlab("Joint name") +
    ggplot2::ylab(xlabel) +
    ggplot2::scale_colour_manual(values = palette_experiences) +
    ggplot2::coord_flip()
  plot(p)
}

# Give palette entries a name so the color of each experience is the same for every graph
plot_palette <- function(experiences) {
  setNames(RColorBrewer::brewer.pal(length(experiences),"Set2"), experiences)
}

plot_aspect <- function(n = 1) {
  return (0.614 + n * 0.04)
}

plot_aspect_graphs <- function(n = 1) {
  return (0.614 * n)
}
