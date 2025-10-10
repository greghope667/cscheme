(define early-macros '())

(define (define-early-macro name transformer)
  (set! early-macros (cons (cons name transformer) early-macros)))

(define (macroexpand expr env)
  (if (null? expr) ()
    (if (list? expr)
      (begin
        (define macro (assq (car expr) early-macros))
        (if macro
          (macroexpand ((cdr macro) (cdr expr)) env)
          (map (lambda (e) (macroexpand e env)) expr)))
      expr)))

(define (eval expr env)
  (define expanded (macroexpand expr env))
  (define compiled (compile expanded env))
  (compiled))

(define-early-macro 'let
  (lambda (body)
    (define names (map car (car body)))
    (define values (map cadr (car body)))
    (define body (cdr body))
    (cons (list 'lambda names (cons 'begin body)) values)))

(define-early-macro 'cond
  (lambda (body)
    (define name (gensym))
    (define (nested name clause rest)
      (list
        'begin
        (list 'set! name (car clause))
        (list 'if name
          (cons 'begin (cdr clause))
          (if (null? rest) (begin)
            (nested name (car rest) (cdr rest))))))
    (if (null? body) (begin)
      (list
        (list 'lambda (list name)
          (nested name (car body) (cdr body)))
        (begin)))))

(define-early-macro 'or
  (lambda (body)
    (define name (gensym))
    (define (nested arg rest)
      (if (null? rest)
        arg
        (list
          'begin
          (list 'set! name arg)
          (list 'if name
            name
            (nested (car rest) (cdr rest))))))
    (if (null? body) #f
      (list
        (list 'lambda (list name)
          (nested (car body) (cdr body)))
        (begin)))))

(define-early-macro 'and
  (lambda (body)
    (define name (gensym))
    (define (nested arg rest)
      (if (null? rest)
        arg
        (list
          'begin
          (list 'set! name arg)
          (list 'if name
            (nested (car rest) (cdr rest))
            #f))))
    (if (null? body) #t
      (list
        (list 'lambda (list name)
          (nested (car body) (cdr body)))
        (begin)))))
