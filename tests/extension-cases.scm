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

(case1
    ; Empty begin -> void
    (begin) ==>? void?)

(cases
    ; Ok, this isn't an extension. But hey, pretty cool!
    (((lambda (q) `(,q ',q)) '(lambda (q) `(,q ',q)))
     ==>
     ((lambda (q) `(,q ',q)) '(lambda (q) `(,q ',q))))

    (((lambda (q) (quasiquote ((unquote q) (quote (unquote q))))) (quote (lambda (q) (quasiquote ((unquote q) (quote (unquote q)))))))
     ==>
     ((lambda (q) (quasiquote ((unquote q) (quote (unquote q))))) (quote (lambda (q) (quasiquote ((unquote q) (quote (unquote q)))))))))

(cases
    ; Multiple values are spliced into argument lists
    ((values 1 2 3) ==> 1 2 3)
    ((list (values 1 2 3)) ==> (1 2 3))
    ((+ (apply values '(1 2 3))) ==> 6)
    ((+ (values (values 1 2))) ==> 3))
