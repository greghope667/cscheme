(section "Extension test cases")

;; Test cases not from the r7rs spec.
;; Some of these are in srfi's, others are a custom extension

(case1
    ; Self evaluating lists
    () ==> ())

(cases
    ; Boolean function
    ((boolean 1) ==> #t)
    ((boolean #f) ==> #f))
