/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ripaparo <ripaparo@student.42madrid.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 13:48:27 by ripaparo          #+#    #+#             */
/*   Updated: 2026/11/10 16:00:00 by ripaparo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>

typedef enum e_sched
{
	SCHED_FIFO,
	SCHED_EDF
}	t_sched;

typedef struct s_config
{
	int			num_coders;
	long long	time_to_burnout;
	long long	time_to_compile;
	long long	time_to_debug;
	long long	time_to_refactor;
	int			num_compiles_required;
	long long	dongle_cooldown;
	t_sched		scheduler;
}	t_config;

typedef struct s_heap_node
{
	int			coder_id;
	long long	request_seq;
	long long	deadline;
}	t_heap_node;

typedef struct s_heap
{
	t_heap_node	*nodes;
	int			size;
	int			capacity;
}	t_heap;

typedef struct s_data	t_data;

typedef struct s_dongle
{
	int				id;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	int				is_in_use;
	long long		cooldown_until;
	t_heap			wait_heap;
}	t_dongle;

typedef struct s_coder
{
	int			id;
	pthread_t	thread;
	int			compile_count;
	long long	last_compile_start;
	int			first_dongle_id;
	int			second_dongle_id;
	t_data		*data;
}	t_coder;

struct s_data
{
	t_config		config;
	long long		start_time;
	int				sim_stopped;
	long long		global_req_seq;
	pthread_mutex_t	state_mutex;
	pthread_mutex_t	print_mutex;
	t_dongle		*dongles;
	t_coder			*coders;
	pthread_t		monitor_thread;
};

/* Parsing */
int			parse_args(t_config *config, int argc, char **argv);

/* Inicialización y Limpieza */
int			init_simulation(t_data *data, int argc, char **argv);
void		cleanup_simulation(t_data *data);

/* Cola de Prioridad (Min-Heap) */
int			heap_init(t_heap *heap, int capacity);
void		heap_destroy(t_heap *heap);
int			heap_push(t_heap *heap, t_heap_node node, t_sched sched);
int			heap_pop(t_heap *heap, t_heap_node *out, t_sched sched);
int			heap_peek(t_heap *heap, t_heap_node *out);

/* Gestión de Dongles */
int			acquire_dongles(t_coder *coder);
void		release_dongles(t_coder *coder);

/* Rutinas Concurrentes */
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);

/* Utilidades de Tiempo y Estado */
long long	get_time_ms(void);
void		precise_sleep(long long ms, t_data *data);
void		print_status(t_coder *coder, const char *status);
int			is_simulation_stopped(t_data *data);

#endif
