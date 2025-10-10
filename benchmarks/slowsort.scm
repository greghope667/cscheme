(define (filter pred ls)
  (if (null? ls) '()
    (if (pred (car ls))
      (cons (car ls) (filter pred (cdr ls)))
      (filter pred (cdr ls)))))

(define (sort ls)
  (if (null? ls) '()
    (let ((p (car ls)))
      (append
        (sort (filter (lambda (v) (< v p)) (cdr ls)))
        (list p)
        (sort (filter (lambda (v) (not (< v p))) (cdr ls)))))))

(define (gen-numbers n z)
  (if (> n 0)
    (cons z (gen-numbers (- n 1) (remainder (+ z 618033) 1000000)))
    '()))

(define x (sort (gen-numbers 20000 0)))
(display (list-ref x 1000))
(newline)
