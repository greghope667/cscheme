;; Incomplete syntax-rules implementation

(define-syntax and-let*
  (ir-macro-transformer
    (lambda (expr rename =?)
      (define args (cadr expr))
      (define body (cddr expr))
      (define name (gensym))
      (list
        (list 'lambda (list name)
          (let loop ((args args))
            (cond
              [(null? args) (cons 'begin body)]
              [(identifier? (caar args))
               `(if (define . ,(car args)) ,(loop (cdr args)) #f)]
              [else
                `(if (set! ,name ,(caar args)) ,(loop (cdr args)) #f)])))
        #f))))

(define-syntax unless
  (ir-macro-transformer
    (lambda (expr rename =?)
      `(if ,(cadr expr) ,(begin) (begin . ,(cddr expr))))))


(define (list-mapn f lss)
  (define (loop acca accd lss)
    (if (null? lss)
      (cons (apply f (reverse acca)) (loop () () (reverse accd)))
      (if (null? (car lss))
        ()
        (loop (cons (caar lss) acca) (cons (cdar lss) accd) (cdr lss)))))
  (cond
    [(null? lss) ()]
    [(null? (cdr lss)) (list-map1 f (car lss))]
    [else (loop () () lss)]))

(define (map f . lss) (list-mapn f lss))

(define ellipsis-match (make-struct-type 'ellipsis 'match))

(define (syntax-pattern-compile pattern ellipsis literals)
  (define keywords '(literal ellipsis escape))

  (define (compile-atom pattern)
    (cond
      [(not (symbol? pattern)) pattern]
      [(memq pattern literals) `(literal ,pattern)]
      ((eq? pattern '_) '_)
      [(eq? pattern ellipsis) (error "invalid ellipsis in pattern")]
      [(memq pattern keywords) `(escape ,pattern)]
      [else pattern]))

  (define (get-bindings pattern)
    (cond
      [(eq? pattern '_) ()]
      [(symbol? pattern) (list pattern)]
      [(not (pair? pattern)) ()]
      [(eq? (car pattern) 'literal) ()]
      [(eq? (car pattern) 'escape) (list (cadr pattern))]
      [(eq? (car pattern) 'ellipsis) (caddr pattern)]
      [else (append (get-bindings (car pattern)) (get-bindings (cdr pattern)))]))

  (define (compile-list pattern)
    (define first (compile (car pattern)))
    (define remainder (cdr pattern))
    (cond
      [(null? remainder) (list first)]
      [(and (pair? remainder) (eq? (car remainder) ellipsis))
       (list 'ellipsis first (get-bindings first))]
      [else (cons first (compile remainder))]))

  (define (compile pattern)
    (if (pair? pattern)
      (compile-list pattern)
      (compile-atom pattern)))

  (let ((p (compile pattern))) (values p (get-bindings p))))


(define (syntax-pattern-match pattern expr)
  (define (match-ellipsis pattern bind-names expr)
    (define (merge-bindings new old)
      (if (pair? new)
        (cons
          (cons* (caar new) (cadar new) (cdar old))
          (merge-bindings (cdr new) (cdr old)))
        ()))

    (define (wrap binding)
      (list (car binding) (make-struct ellipsis-match (reverse (cdr binding)))))

    (let loop ([expr expr] [bindings (map list bind-names)])
      (cond
        [(null? expr) (map wrap bindings)]
        [(not (pair? expr)) #f]
        [(match pattern (car expr))
         => (lambda (matched-bindings)
               (loop
                 (cdr expr)
                 (merge-bindings matched-bindings bindings)))]
        [else #f])))

  (define (match pattern expr)
    (cond
      [(eq? pattern '_) ()]
      [(symbol? pattern) (cons (list pattern expr) ())]
      [(not (pair? pattern)) (if (eqv? pattern expr) () #f)]
      [(eq? (car pattern) 'literal) (and (symbol-identifier=? (cadr pattern) expr) ())]
      [(eq? (car pattern) 'escape) (cons (list (cadr pattern) expr) ())]
      [(eq? (car pattern) 'ellipsis) (match-ellipsis (cadr pattern) (caddr pattern) expr)]
      [else
        (and-let*
          ([(pair? expr)]
           [a (match (car pattern) (car expr))]
           [d (match (cdr pattern) (cdr expr))])
          (append a d))]))

  (match pattern expr))


(define (syntax-template-compile template ellipsis pattern-vars)
  (define keywords '(ellipsis escape literal))

  (define (compile-atom template ellipsis)
    (cond
      [(not (symbol? template)) template]
      [(eq? template ellipsis) (error "invalid ellipsis in template")]
      [(memq template keywords)
       (list (if (memq template pattern-vars) 'escape 'literal) template)]
      [(memq template pattern-vars) template]
      [else (list 'literal template)]))

  (define (get-bindings template)
    (cond
      [(symbol? template) (list template)]
      [(not (pair? template)) ()]
      [(eq? (car template) 'escape) (list (cadr template))]
      [(eq? (car template) 'ellipsis) (caddr template)]
      [(eq? (car template) 'literal) ()]
      [else (append (get-bindings (car template)) (get-bindings (cdr template)))]))

  (define (compile-list template ellipsis)
    (define first (compile (car template) ellipsis))
    (define remainder (cdr template))
    (cond
      [(null? remainder) (list first)]
      [(and (pair? remainder) (eq? (car remainder) ellipsis))
       (let ((bindings (get-bindings first)))
        (if (null? bindings)
          (error "template has no bindings" template)
          (list 'ellipsis first (get-bindings first))))]
      [else (cons first (compile remainder ellipsis))]))

  (define (compile template ellipsis)
    (if (pair? template)
      (compile-list template ellipsis)
      (compile-atom template ellipsis)))

  (compile template ellipsis))


(define (syntax-template-expand template match scope)
  (define (expand-ellipsis template bindings match)
    (define (unwrap b) (struct-ref (cadr (assq b match)) 0 ellipsis-match))
    (define new-values (map unwrap bindings))
    (unless (apply = (map list-length new-values))
      (error "Ellipsis length mismatch" new-values))
    (list-mapn
      (lambda vs (expand template (map list bindings vs)))
      new-values))

  (define (expand template match)
    (cond
      [(symbol? template) (cadr (assq template match))]
      [(not (pair? template)) template]
      [(eq? (car template) 'literal) (add-scope (cadr template) scope)]
      [(eq? (car template) 'escape) (cadr (assq (cadr template) match))]
      [(eq? (car template) 'ellipsis) (expand-ellipsis (cadr template) (caddr template) match)]
      [else (cons (expand (car template) match) (expand (cdr template) match))]))

  (expand template match))


(define (syntax-rules-expander patterns+templates)
  (lambda (expr caller-env macro-env)
    (let loop ([p+t patterns+templates])
      (if (null? p+t)
        (error "syntax-rules failed to match any patterns" (remove-scope expr))
        (let ([match (syntax-pattern-match (caar p+t) expr)])
          (if match
            (syntax-template-expand (cdar p+t) match macro-env)
            (loop (cdr p+t))))))))


(define-syntax syntax-rules
  ((lambda ()
    (define pattern
      (car (list (syntax-pattern-compile '(_ (literals ...) (pattern template) ...) '... ()))))
    (lambda (expr caller-env macro-env)
      (set! expr (remove-scope expr))
      (define match (syntax-pattern-match pattern expr))
      (unless match (error "Invalid syntax-rules form" expr))
      (define (get part) (struct-ref (cadr (assq part match)) 0 ellipsis-match))
      (define literals (get 'literals))
      (syntax-rules-expander
        (map
         (lambda (pat templ)
          ((lambda (cpattern pattern-vars)
            (define ctemplate (syntax-template-compile templ '... pattern-vars))
            (cons cpattern ctemplate))
           (syntax-pattern-compile pat '... literals)))
         (get 'pattern) (get 'template)))))))
