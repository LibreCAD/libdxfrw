/*
 * Create a local-only AC1015 control DWG with non-default OCS normals.
 *
 * This uses LibreDWG's public API because the dwgadd text recipe can set
 * scalar fields but cannot set the ARC/CIRCLE extrusion vectors. The output
 * belongs in the build tree and is never committed as a DWG fixture.
 */
#include <dwg.h>
#include <dwg_api.h>

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  Dwg_Data *dwg;
  Dwg_Object *model_space;
  Dwg_Object_BLOCK_HEADER *block_header;
  Dwg_Entity_ARC *arc;
  Dwg_Entity_CIRCLE *circle;
  const dwg_point_3d arc_center = { 10.0, 20.0, 30.0 };
  const dwg_point_3d circle_center = { -5.0, 4.0, 10.0 };
  const BITCODE_BE extrusion = { 0.6, 0.0, 0.8 };
  int result = EXIT_FAILURE;

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s OUTPUT.dwg\n", argv[0]);
      return EXIT_FAILURE;
    }

  dwg = dwg_new_Document (R_2000, 0, 0);
  if (!dwg)
    {
      fprintf (stderr, "LibreDWG could not create an AC1015 document\n");
      return EXIT_FAILURE;
    }

  model_space = dwg_model_space_object (dwg);
  block_header = model_space ? model_space->tio.object->tio.BLOCK_HEADER : NULL;
  if (!block_header)
    {
      fprintf (stderr, "LibreDWG did not create model space\n");
      goto cleanup;
    }

  arc = dwg_add_ARC (block_header, &arc_center, 5.0, 0.0, 1.5707963267948966);
  circle = dwg_add_CIRCLE (block_header, &circle_center, 2.5);
  if (!arc || !circle)
    {
      fprintf (stderr, "LibreDWG could not add ARC and CIRCLE controls\n");
      goto cleanup;
    }

  arc->extrusion = extrusion;
  circle->extrusion = extrusion;

  if (dwg_write_file (argv[1], dwg) != 0)
    {
      fprintf (stderr, "LibreDWG failed to write %s\n", argv[1]);
      goto cleanup;
    }
  result = EXIT_SUCCESS;

cleanup:
  dwg_free (dwg);
  free (dwg);
  return result;
}
