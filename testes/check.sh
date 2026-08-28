#!/bin/bash
# Testes oficiais da disciplina.
#
# Cada caso roda o programa e compara TODOS os arquivos .pgm gerados contra o
# mesmo gabarito. Isso cobre duas exigencias do enunciado de uma vez: cada
# implementacao bate com o esperado, e as quatro sao identicas entre si.
#
# Funciona em qualquer estagio do desenvolvimento: verifica os arquivos que
# existem, sem precisar ser editado a cada implementacao nova.

cd "$(dirname "$0")/.." || exit 1

falhas=0

caso() {
    esperado="$1"
    shift
    echo "===== ./mandelbrot $* ====="

    rm -f mandelbrot_mrbm_*.pgm

    if ! ./mandelbrot "$@"; then
        echo "FALHOU: execucao terminou com status $?"
        falhas=$((falhas + 1))
        return
    fi

    encontrados=0
    for f in mandelbrot_mrbm_*.pgm; do
        [ -e "$f" ] || continue
        encontrados=$((encontrados + 1))
        if diff -u "$esperado" "$f"; then
            echo "OK: $f"
        else
            echo "FALHOU: $f difere do gabarito"
            falhas=$((falhas + 1))
        fi
    done

    if [ "$encontrados" -eq 0 ]; then
        echo "FALHOU: nenhum arquivo .pgm foi gerado"
        falhas=$((falhas + 1))
    fi
}

caso testes/esperado/teste1.txt 4 4 50 1
caso testes/esperado/teste2.txt 6 6 30 3
caso testes/esperado/teste3.txt 10 6 40 4

# O enunciado exige que nada seja impresso em stdout na execucao normal.
echo "===== stdout mudo ====="
saida=$(./mandelbrot 4 4 50 1)
if [ -z "$saida" ]; then
    echo "OK: stdout vazio"
else
    echo "FALHOU: stdout imprimiu:"
    echo "$saida"
    falhas=$((falhas + 1))
fi

# times.txt precisa existir e trazer uma linha por implementacao.
echo "===== times.txt ====="
if [ -s times.txt ]; then
    cat times.txt
    echo "OK: times.txt gerado"
else
    echo "FALHOU: times.txt ausente ou vazio"
    falhas=$((falhas + 1))
fi

rm -f mandelbrot_mrbm_*.pgm times.txt

echo
if [ "$falhas" -eq 0 ]; then
    echo "TODOS OS TESTES PASSARAM"
else
    echo "$falhas verificacao(oes) falharam"
fi
exit "$falhas"
