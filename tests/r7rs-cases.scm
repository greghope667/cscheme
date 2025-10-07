(section "R7RS test cases")

;; This file contains a bunch of code snippets from the
;; r7rs reference manual, to be used as tests

(section "2 Lexical conventions")

; Line comments are ignored by the reader.

; #| block comments are not yet implemented |#
; #;(datum comments not yet implemented)
(skip "block comments" "datum comments")

(section "4.1 Primative expression types")

(case1
  (begin
    (define x 28)
    x)
  ==> 28)

(cases
  ((quote a) ==> a)
  ;((quote #(a b c)) ==> #(a b c))
  ((quote (+ 1 2)) ==> (+ 1 2)))

(cases
  ('a ==> a)
  ;('#(a b c) ==> #(a b c))
  ('() ==> ())
  ('(+ 1 2) ==> (+ 1 2))
  ('(quote a) ==> (quote a))
  (''a ==> (quote a)))

(cases
  ('145932 ==> 145932)
  (145932 ==> 145932)
  ('"abc" ==> "abc")
  ("abc" ==> "abc")
  ('#\a ==> #\a)
  (#\a ==> #\a)
  ;('#(a 10) ==> #(a 10))
  ;(#(a 10) ==> #(a 10))
  ;('#u8(64 65) ==> #(64 65))
  ;(#u8(64 65) ==> #(64 65))
  ('#t ==> #t)
  (#t ==> #t))


(section "6.1 Equivalence predicates")

(cases
  ((eqv? 'a 'a) ==> #t)
  ((eqv? 'a 'b) ==> #f)
  ((eqv? 2 2) ==> #t)
  ;((eqv? 2 2.0) ==> #f)
  ((eqv? '() '()) ==> #t)
  ((eqv? 100000000 100000000) ==> #t)
  ;((eqv? 0.0 +nan.0) ==> #f)
  ((eqv? (cons 1 2) (cons 1 2)) ==> #f)
  ((eqv? (lambda () 1) (lambda () 2)) ==> #f)
  ;((let ([p (lambda (x) x)])
  ;    (eqv? p p)) ==> #t)
  ((eqv? #f 'nil) ==> #f))

(cases
  ((eqv? "" "") ==>? boolean?)
  ;((eqv? '#() '#()) ==>? boolean?)
  ((eqv? (lambda (x) x) (lambda (x) x)) ==>? boolean?)
  ((eqv? (lambda (x) x) (lambda (y) y)) ==>? boolean?))
  ;((eqv? 1.0e0 1.0e0) ==>? boolean?)
  ;((eqv? +nan.0 +nan.0) ==>? boolean?))

;(cases)
  ;((let ([x '(a)]) (eqv? x x)) ==> #t))

(cases
  ((eq? 'a 'a) ==> #t)
  ((eq? '(a) '(a)) ==>? boolean?)
  ((eq? (list 'a) (list 'a)) ==> #f)
  ((eq? "a" "a") ==>? boolean?)
  ((eq? "" "") ==>? boolean?)
  ((eq? '() '()) ==> #t)
  ((eq? 2 2) ==>? boolean?)
  ((eq? #\A #\A) ==>? boolean?)
  ((eq? car car) ==> #t))
  ;((let ([n (+ 2 3)]) (eq? n n)) ==>? boolean?)
  ;((let ([x '(a)]) (eq? x x)) ==> #t))

(cases
  ((equal? 'a 'a) ==> #t)
  ((equal? '(a) '(a)) ==> #t)
  ((equal? '(a (b) c) '(a (b) c)) ==> #t)
  ((equal? "abc" "abc") ==> #t)
  ((equal? 2 2) ==> #t))
  ;((equal? (make-vector 5 'a) (make-vector 5 'a)) ==> #t))

(section "6.3 Booleans")

(cases
  (#t ==> #t)
  (#f ==> #f)
  ('#t ==> #t))

(cases
  ((not #t) ==> #f)
  ((not 3) ==> #f)
  ((not (list 3)) ==> #f)
  ((not #f) ==> #t)
  ((not '()) ==> #f)
  ((not (list)) ==> #f)
  ((not 'nil) ==> #f))

(cases
  ((boolean? #t) ==> #t)
  ((boolean? #f) ==> #t)
  ((boolean? 0) ==> #f)
  ((boolean? '()) ==> #f))

(cases
  ((boolean=? #t #t #t) ==> #t)
  ((boolean=? #f #f) ==> #t)
  ((boolean=? #t #f) ==> #f))

(section "6.4 Pairs and lists")

(cases
  ((pair? '(a . b)) ==> #t)
  ((pair? '(a b c)) ==> #t)
  ((pair? '()) ==> #f))
  ;((pair? '#(a b) ==> #f)))

(cases
  ((cons 'a '()) ==> (a))
  ((cons '(a) '(b c d)) ==> ((a) b c d))
  ((cons "a" '(b c)) ==> ("a" b c))
  ((cons 'a 3) ==> (a . 3))
  ((cons '(a b) 'c) ==> ((a b) . c)))

(cases
  ((car '(a b c)) ==> a)
  ((car '((a) b c d)) ==> (a))
  ((cdr '((a) b c d)) ==> (b c d))
  ((car '(1 . 2)) ==> 1)
  ((cdr '(1 . 2)) ==> 2))

(case1
  (begin
    (define x (cons 1 2))
    (set-car! x 3)
    (set-cdr! x 4)
    x)
  ==> (3 . 4))

(cases
  ((car '(1 (2 3) 4 5)) ==> 1)
  ((cadr '(1 (2 3) 4 5)) ==> (2 3))
  ((caddr '(1 (2 3) 4 5)) ==> 4)
  ((caadr '(1 (2 3) 4 5)) ==> 2)
  ((cdadr '(1 (2 3) 4 5)) ==> (3))
  ((cdddr '(1 (2 3) 4 5)) ==> (5)))

(cases
  ((null? '()) ==> #t)
  ((null? '(1 2)) ==> #f)
  ((list? '()) ==> #t)
  ((list? '(1 2)) ==> #t))

(skip
  ; Circular lists not yet handled
  (let ([x (list 'a)])
    (set-cdr! x x)
    (list? x))
  ==> #f)

(cases
  ((make-list 2 3) ==> (3 3)))

(cases
  ((length '(a b c)) ==> 3)
  ((length '(a (b) (c d e))) ==> 3)
  ((length '()) ==> 0))

(cases
  ((append '(x) '(y)) ==> (x y))
  ((append '(a) '(b c d)) ==> (a b c d))
  ((append '(a (b)) '((c))) ==> (a (b) (c)))
  ((append '(a b) '(c . d)) ==> (a b c . d))
  ((append '() 'a) ==> a))

(cases
  ((reverse '(a b c)) ==> (c b a))
  ((reverse '(a (b c) d (e (f)))) ==> ((e (f)) d (b c) a)))

(cases
  ((list-tail '() 0) ==> ())
  ((list-tail '(a b c d e) 2) ==> (c d e)))

(cases
  ((list-ref '(a b c d) 2) ==> c)
  ((begin
    (define l (list 'a 'b 'e))
    (list-set! l 2 'c)
    l) ==> (a b c)))
