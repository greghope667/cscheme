;; Some useful list-manipulation functions, mostly from srfi-1.
(define (cddr x) (general-car-cdr x #b100))
(define (cdar x) (general-car-cdr x #b101))
(define (cadr x) (general-car-cdr x #b110))
(define (caar x) (general-car-cdr x #b111))

(define (cdddr x) (general-car-cdr x #b1000))
(define (cddar x) (general-car-cdr x #b1001))
(define (cdadr x) (general-car-cdr x #b1010))
(define (cdaar x) (general-car-cdr x #b1011))
(define (caddr x) (general-car-cdr x #b1100))
(define (cadar x) (general-car-cdr x #b1101))
(define (caadr x) (general-car-cdr x #b1110))
(define (caaar x) (general-car-cdr x #b1111))

(define (list-map1 f ls)
  (if (null? ls) ()
    (cons (f (car ls)) (list-map1 f (cdr ls)))))

(define (list-find-tail proc ls)
  (if (pair? ls)
    (if (proc (car ls))
      ls
      (list-find-tail proc (cdr ls)))
    #f))

(define (list-find proc ls)
  (define tail (list-find-tail proc ls))
  (if tail (car tail) #f))

(define (member key list cmp)
  (list-find-tail (lambda (v) (cmp key v)) list))

(define (memq key list) (member key list eq?))
(define (memv key list) (member key list eqv?))

(define (assoc key alist cmp)
  (list-find (lambda (v) (cmp key (car v))) alist))

(define (assq key alist) (assoc key alist eq?))
(define (assv key alist) (assoc key alist eqv?))

(define map list-map1)

(define (pair-map fn pair) (cons (fn (car pair)) (fn (cdr pair))))
