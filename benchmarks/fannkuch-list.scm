(define (iota count start)
  (if (> count 0)
    (cons start (iota (- count 1) (+ start 1)))
    '()))

(define (list-remove n ls)
  (cond
    ((null? ls) '())
    ((eqv? n (car ls)) (cdr ls))
    (else (cons (car ls) (list-remove n (cdr ls))))))

(define (list-each f ls)
  (if (null? ls) #f
    (begin (f (car ls)) (list-each f (cdr ls)))))

(define (each-permutation f ls)
  (if (null? ls) (f '())
    (list-each
      (lambda (x)
        (each-permutation
          (lambda (l) (f (cons x l)))
          (list-remove x ls)))
      ls)))

(define (flip-head! n ls)
  (define split (list-tail ls (- n 1)))
  (define tail (cdr split))
  (set-cdr! split '())
  (append (reverse ls) tail))

(define (fannkuch-list ls)
  (let loop ((ls ls) (n 0))
    (if (eqv? (car ls) 1) n
      (loop (flip-head! (car ls) ls) (+ n 1)))))

(define (fannkuch-max ls)
  (define m 0)
  (each-permutation
    (lambda (p)
      (define n (fannkuch-list p))
      (if (> n m) (set! m n)))
    ls)
  m)

(display (fannkuch-max (iota 8 1)))
(newline)
