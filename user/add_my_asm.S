.data
#define BUFF_LEN 256
buff: .space BUFF_LEN
fmt: .asciz "%d\n"
err_no_space: .asciz "No space in input\n"
err_extra: .asciz "Extra characters in input\n"
err_empty_input: .asciz "Empty input\n"
err_invalid: .asciz "Invalid number format\n"
err_overflow:.asciz "Buffer size exceeded\n"

.text
.global main

main: 
    addi sp, sp, -16
    sw   s0, 0(sp)
    sw   s1, 4(sp)
    sw   s2, 8(sp)
    sw   s3, 12(sp)

    # Считываем всю строку с клавиатуры через gets(buff, BUFF_LEN)
    la a0, buff
    li a1, BUFF_LEN
    call gets

    # Проверка на пустой ввод
    la a0, buff
    call strlen
    beq a0, x0, error_empty_input

    # Проверка на переполнение 
    la t2, buff
    li t3, BUFF_LEN - 1
    li t4, 0
check_overflow:
    bge t4, t3, error_overflow # Если количество символов больше, чем BUFF_LEN - 1, то ошибка
    lb t0, 0(t2)
    beq t0, x0, after_overflow # Встретили терминирующий нуль, переходим дальше по программе
    addi t4, t4, 1
    addi t2, t2, 1
    j check_overflow

after_overflow:
    # Поиск пробела в строке 
    la a0, buff
    li a1, 32 # ASCII код пробела
    call strchr 
    beq a0, x0, error_no_space # Если нет пробела, то ошибка
    move s3, a0 # s3 = указатель на пробел

    # После одного пробела не должно быть второго (т.е. ровно 2 аргумента)
    addi a0, s3, 1
    li a1, 32
    call strchr
    bne a0, x0, error_extra # Если есть второй пробел, то ошибка
    
    # Ставим нулевой символ вместо первого пробела
    li t0, 0
    sb t0, 0(s3)

    # Проверяем, что первая подстрока не пустая
    la a0, buff
    call strlen
    beq a0, x0, error_invalid

    # Проверяем, что вторая подстрока не пустая
    addi a0, s3, 1
    call strlen
    beq a0, x0, error_invalid

    # Проверяем, что первая подстрока - число
    la a0, buff
    call check_digits

    # Очистка второй подстроки от символов новой строки
remove_newline:
    addi a0, s3, 1 # a0 = указатель на начало второй подстроки
remove_newline_loop:
    lb t0, 0(a0)
    beq t0, x0, end_remove_newline  # если достигнут конец строки, выходим
    li t1, 10             # ASCII для '\n'
    beq t0, t1, set_null  # если нашли '\n', заменяем
    addi a0, a0, 1
    j remove_newline_loop
set_null:
    li t0, 0
    sb t0, 0(a0)
end_remove_newline:

    # Проверяем, что вторая подстрока - число
    addi a0, s3, 1
    call check_digits

    # Преобразуем первую подстроку в число
    la a0, buff
    call atoi
    move s0, a0

    # Преобразуем вторую подстроку в число
    addi a0, s3, 1
    call atoi
    move s1, a0

    # Складываем два числа
    add s2, s0, s1

    # Выводим
    la a0, fmt 
    move a1, s2
    call printf

    lw s0, 0(sp)
    lw s1, 4(sp)
    lw s2, 8(sp)
    lw s3, 12(sp)
    addi sp, sp, 16

    # Завершаем программу
    li a7, 2
    li a0, 0
    ecall

error_no_space:
    la a0, err_no_space
    call printf
    li a7, 2
    li a0, 1
    ecall

error_extra:
    la a0, err_extra
    call printf
    li a7, 2
    li a0, 1
    ecall

error_empty_input:
    la a0, err_empty_input
    call printf
    li a7, 2
    li a0, 1
    ecall

error_overflow:
    la a0, err_overflow
    call printf
    li a7, 2
    li a0, 1
    ecall

error_invalid:
    la a0, err_invalid
    call printf
    li a7, 2
    li a0, 1
    ecall

check_digits:
    lb t0, 0(a0)
    li t1, 45  # '-'
    beq t0, t1, check_digits_negative # Проверка для отрицательных чисел
    j check_digits_loop
check_digits_negative:
    addi a0, a0, 1  # Пропускаем '-'
    lb t0, 0(a0)  # После '-' должен быть хотя бы один символ
    beq t0, x0, error_invalid
    j check_digits_loop
check_digits_loop:
    lb t0, 0(a0)
    beq t0, x0, check_digits_end
    li t1, 48
    blt t0, t1, error_invalid # если символ меньше '0', ошибка
    li t1, 57
    bgt t0, t1, error_invalid # если символ больше '9', ошибка
    addi a0, a0, 1
    j check_digits_loop
check_digits_end:
    jr ra
