;; Locally-authored, known-size ACIS/ACDS witness generator.
;; Run only in full AutoCAD for Windows (not AutoCAD LT).
;; See acis218_autocad_witness.md for evidence and handling requirements.

(vl-load-com)

(defun libdxfrw-acis-join-path (directory filename)
  (strcat (vl-string-right-trim "\\/" directory) "\\" filename)
)

(defun libdxfrw-acis-absolute-path-p (path)
  (or
    (and
      (>= (strlen path) 3)
      (= (substr path 2 1) ":")
      (member (substr path 3 1) '("\\" "/")))
    (and
      (>= (strlen path) 3)
      (= (substr path 1 2) "\\\\")
      (/= (substr path 3 1) ""))
  )
)

(defun libdxfrw-acis-array-list (value)
  (if (= (type value) 'VARIANT)
    (vlax-safearray->list (vlax-variant-value value))
    (vlax-safearray->list value)
  )
)

(defun libdxfrw-acis-near (actual expected tolerance)
  (< (abs (- actual expected)) tolerance)
)

(defun libdxfrw-acis-point-near (actual expected tolerance)
  (and
    (libdxfrw-acis-near (car actual) (car expected) tolerance)
    (libdxfrw-acis-near (cadr actual) (cadr expected) tolerance)
    (libdxfrw-acis-near (caddr actual) (caddr expected) tolerance)
  )
)

(defun libdxfrw-acis-solid-extents (solid / minimum maximum)
  (vla-GetBoundingBox solid 'minimum 'maximum)
  (list
    (libdxfrw-acis-array-list minimum)
    (libdxfrw-acis-array-list maximum)
  )
)

(defun libdxfrw-acis-write-value (stream key value)
  (write-line (strcat key "=" (vl-princ-to-string value)) stream)
)

(defun libdxfrw-acis-save-as (document path file-type / result)
  (setq result
    (vl-catch-all-apply 'vla-SaveAs (list document path file-type)))
  (if (vl-catch-all-error-p result)
    (progn
      (prompt (strcat "\nSaveAs failed for " path ": "
                      (vl-catch-all-error-message result)))
      nil)
    T)
)

(defun c:LIBDXFRWMAKEACISWITNESS
  (/ directory names paths acad original-document document model-space solid
     report minimum maximum extents expected-minimum expected-maximum volume
     product-version acad-version product-build output-path report-stream
     units-ok geometry-ok)
  (setq directory (getstring T "\nOutput directory (absolute path; outputs must not exist): "))
  (cond
    ((not (libdxfrw-acis-absolute-path-p directory))
      (prompt "\nEnter an absolute Windows drive or UNC path; no drawing was created."))
    ((not (vl-file-directory-p directory))
      (prompt "\nThat directory does not exist; no drawing was created."))
    (T
      (setq names
        '("acis218_box_mm.dwg" "acis218_box_mm.dxf"
          "acis218_box_in.dwg" "acis218_box_in.dxf"
          "acis218_box_manifest.txt"))
      (setq paths (mapcar '(lambda (name) (libdxfrw-acis-join-path directory name)) names))
      (if (vl-some 'findfile paths)
        (prompt "\nAt least one output already exists; choose an empty directory. No file was overwritten.")
        (progn
          (setq acad (vlax-get-acad-object))
          (setq original-document (vla-get-ActiveDocument acad))
          (setq document (vla-Add (vla-get-Documents acad)))
          (vla-Activate document)
          (setq model-space (vla-get-ModelSpace document))
          (vla-SetVariable document "INSUNITS" 4)
          ;; ActiveX AddBox takes the center point, followed by X/Y/Z lengths.
          (setq solid
            (vla-AddBox model-space (vlax-3d-point 13.0 -17.0 19.0)
                        5.0 7.0 11.0))
          (setq extents (libdxfrw-acis-solid-extents solid))
          (setq minimum (car extents))
          (setq maximum (cadr extents))
          (setq expected-minimum '(10.5 -20.5 13.5))
          (setq expected-maximum '(15.5 -13.5 24.5))
          (setq volume (vla-get-Volume solid))
          (setq geometry-ok
            (and
              (libdxfrw-acis-point-near minimum expected-minimum 1e-9)
              (libdxfrw-acis-point-near maximum expected-maximum 1e-9)
              (libdxfrw-acis-near volume 385.0 1e-8)))
          (setq report-stream (open (nth 4 paths) "w"))
          (if (not report-stream)
            (prompt "\nCould not create manifest; close the empty drawing and retry.")
            (progn
              (setq product-version (vla-get-Version acad))
              (setq acad-version (getvar "ACADVER"))
              (setq product-build (getvar "_VERNUM"))
              (libdxfrw-acis-write-value report-stream "producer" "Autodesk AutoCAD ActiveX")
              (libdxfrw-acis-write-value report-stream "product_version" product-version)
              (libdxfrw-acis-write-value report-stream "acadver" acad-version)
              (libdxfrw-acis-write-value report-stream "product_build" product-build)
              (libdxfrw-acis-write-value report-stream "target_dwg_dxf_version" "AutoCAD 2013 / AC1027")
              (libdxfrw-acis-write-value report-stream "entity" "one 3DSOLID created by ModelSpace.AddBox")
              (libdxfrw-acis-write-value report-stream "box_center_wcs" '(13.0 -17.0 19.0))
              (libdxfrw-acis-write-value report-stream "box_lengths_xyz" '(5.0 7.0 11.0))
              (libdxfrw-acis-write-value report-stream "expected_min_wcs" expected-minimum)
              (libdxfrw-acis-write-value report-stream "expected_max_wcs" expected-maximum)
              (libdxfrw-acis-write-value report-stream "autocad_getboundingbox_min_wcs" minimum)
              (libdxfrw-acis-write-value report-stream "autocad_getboundingbox_max_wcs" maximum)
              (libdxfrw-acis-write-value report-stream "autocad_solid_volume" volume)
              (libdxfrw-acis-write-value report-stream "coordinates_oriented_by_insunits" "no; changing INSUNITS does not transform geometry")
              (if geometry-ok
                (progn
                  (setq units-ok
                    (= (vlax-variant-value
                         (vla-GetVariable document "INSUNITS")) 4))
                  (if units-ok
                    (progn
                      (setq output-path (car paths))
                      (setq units-ok (libdxfrw-acis-save-as document output-path 60))
                      (if units-ok
                        (setq units-ok
                          (libdxfrw-acis-save-as document (cadr paths) 61)))
                      ;; Change only drawing insertion-unit metadata, then confirm
                      ;; AutoCAD's own solid extents and volume did not move.
                      (if units-ok
                        (progn
                          (vla-SetVariable document "INSUNITS" 1)
                          (setq units-ok
                            (= (vlax-variant-value
                                 (vla-GetVariable document "INSUNITS")) 1))))
                      (if units-ok
                        (progn
                          (setq extents (libdxfrw-acis-solid-extents solid))
                          (setq minimum (car extents))
                          (setq maximum (cadr extents))
                          (setq volume (vla-get-Volume solid))
                          (setq geometry-ok
                            (and
                              (libdxfrw-acis-point-near minimum expected-minimum 1e-9)
                              (libdxfrw-acis-point-near maximum expected-maximum 1e-9)
                              (libdxfrw-acis-near volume 385.0 1e-8))))
                        (setq geometry-ok nil))
                      (libdxfrw-acis-write-value report-stream "inch_variant_insunits"
                        (vlax-variant-value (vla-GetVariable document "INSUNITS")))
                      (libdxfrw-acis-write-value report-stream "inch_variant_getboundingbox_min_wcs" minimum)
                      (libdxfrw-acis-write-value report-stream "inch_variant_getboundingbox_max_wcs" maximum)
                      (libdxfrw-acis-write-value report-stream "inch_variant_solid_volume" volume)
                      (if (and units-ok geometry-ok)
                        (progn
                          (setq output-path (nth 2 paths))
                          (setq units-ok (libdxfrw-acis-save-as document output-path 60))
                          (if units-ok
                            (setq units-ok
                              (libdxfrw-acis-save-as document (nth 3 paths) 61)))
                          (if units-ok
                            (progn
                              (libdxfrw-acis-write-value report-stream "generation_status" "PASS")
                              (prompt (strcat "\nCreated the AC1027 witness pairs in " directory)))
                            (libdxfrw-acis-write-value report-stream "generation_status" "FAIL: AutoCAD could not save all four outputs")))
                        (progn
                          (libdxfrw-acis-write-value report-stream "generation_status"
                            (if units-ok
                              "FAIL: geometry changed after INSUNITS change"
                              "FAIL: could not set INSUNITS or save millimetre outputs"))
                          (prompt "\nUnits, native geometry, or save verification failed; do not use this witness.")))
                    )
                    (progn
                      (libdxfrw-acis-write-value report-stream "generation_status" "FAIL: could not set millimetre INSUNITS")
                      (prompt "\nCould not set millimetre INSUNITS; do not use this witness.")))
                )
                (progn
                  (libdxfrw-acis-write-value report-stream "generation_status" "FAIL: native extents/volume differ from specified box")
                  (prompt "\nAutoCAD extents/volume did not match the requested box; do not use this witness.")))
              (close report-stream)
            )
          )
          (if document (vla-Close document :vlax-false))
          (if original-document (vla-Activate original-document))
        )
      )
    )
  )
  (princ)
)

(prompt "\nLoaded LIBDXFRWMAKEACISWITNESS. It creates and closes its own new drawing.")
(princ)
