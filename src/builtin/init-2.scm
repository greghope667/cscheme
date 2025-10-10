(define (qq-expand-term t)
  (cond
    ((symbol? t) (list 'quote t))
    ((pair? t) (qq-expand-list t))
    (else t)))

(define (qq-expand-list ls)
  (define head (car ls))
  (cond
    ((eq? head 'unquote) (cons values (cdr ls)))
    ((eq? head 'unquote-splicing) (cons apply (cons values (cdr ls))))
    (else (cons list (map qq-expand-term ls)))))

(define (qq-expand expr)
  (cond
    ((not (pair? expr)) expr)
    ((eq? (car expr) 'quasiquote) (qq-expand-term (cadr expr)))
    ((eq? (car expr) 'quote) expr)
    (else (map qq-expand expr))))
