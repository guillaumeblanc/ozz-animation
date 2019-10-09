#!/usr/bin/env Rscript

library("optparse")

parse_commandline <- function() {
  option_list = list(
    make_option(c("-p", "--path"), type="character", default=".",
                help="Experiences file path", metavar="character"),
    make_option(c("-o", "--out"), type="character", default="report.html",
                help="HTML output filename [default= %default]", metavar="character")
  );

  opt_parser = OptionParser(option_list=option_list);
  opt = parse_args(opt_parser);

  if (!dir.exists(opt$path)){
    print_help(opt_parser)
    stop("Experiences path isn't valid.", call.=FALSE)
  }
  return (opt)
}

options <- parse_commandline()

rmd_file <- system.file("rmd", "report.Rmd", package = "ozzr")
rmarkdown::render(input = rmd_file, output_file=options$out, params=list(path=options$path))
