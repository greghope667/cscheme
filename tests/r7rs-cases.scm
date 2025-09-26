(section "Example cases")

(cases
    ; this is a comment that won't execute
    (((lambda (x) x) 3) ==> 3)
    ('(1 2 3) ==>? list?)
    (() ==>? null?)
    ('(1 . 2) ==>? pair?)
    (((lambda (x) x) 4) unspecified))
