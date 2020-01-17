static void test_region_labeling(void) {
  RDImage *image = load_image_fixture("16x16_region_labeling_spiral.raw");
  RDMatrix *regions = load_matrix_fixture("16x16_region_labeling_spiral_labeled");

  RDMatrix *labeled = rd_label_image_regions(image);

  assert_matrix_eq(regions, labeled);

  rd_matrix_free(labeled);
  rd_matrix_free(regions);
  rd_matrix_free(image);
}

static void test_region_labeling_allocation(void)
{
  RDImage *image = load_image_fixture("16x16_region_labeling_spiral.raw");
  RDMatrix *labeled;

  for(int i = 1; i < 3; i++) {
    fail_nth_allocation = i;
    labeled = rd_label_image_regions(image);

    g_assert_null(labeled);
  }

  rd_matrix_free(image);
}
