/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ripaparo <ripaparo@student.42madrid.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 16:36:56 by ripaparo          #+#    #+#             */
/*   Updated: 2026/11/17 15:45:00 by ripaparo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static int	is_valid_numeric(const char *str)
{
	int	i;

	if (!str || !str[0])
		return (0);
	i = 0;
	if (str[i] == '+')
		i++;
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

static int	parse_safe_long(const char *str, long long *out)
{
	long long	val;
	int			i;
	int			digit;

	val = 0;
	i = 0;
	if (str[i] == '+')
		i++;
	while (str[i])
	{
		digit = str[i] - '0';
		if (val > (9223372036854775807LL - digit) / 10)
			return (0);
		val = val * 10 + digit;
		i++;
	}
	*out = val;
	return (1);
}

static int	parse_scheduler(t_config *cfg, const char *str)
{
	if (!str)
		return (0);
	if (strcmp(str, "fifo") == 0)
	{
		cfg->scheduler = POLICY_FIFO;
		return (1);
	}
	if (strcmp(str, "edf") == 0)
	{
		cfg->scheduler = POLICY_EDF;
		return (1);
	}
	return (0);
}

static int	parse_numeric_args(t_config *cfg, char **argv)
{
	long long	vals[7];
	int			i;

	i = 0;
	while (i < 7)
	{
		if (!is_valid_numeric(argv[i + 1]))
			return (0);
		if (!parse_safe_long(argv[i + 1], &vals[i]))
			return (0);
		i++;
	}
	if (vals[0] <= 0 || vals[0] > 2147483647)
		return (0);
	if (vals[5] < 0 || vals[5] > 2147483647)
		return (0);
	cfg->num_coders = (int)vals[0];
	cfg->time_to_burnout = vals[1];
	cfg->time_to_compile = vals[2];
	cfg->time_to_debug = vals[3];
	cfg->time_to_refactor = vals[4];
	cfg->num_compiles_required = (int)vals[5];
	cfg->dongle_cooldown = vals[6];
	return (1);
}

int	parse_args(t_config *config, int argc, char **argv)
{
	if (!config || argc != 9)
	{
		fprintf(stderr, "Error: invalid number of arguments.\n");
		fprintf(stderr, "Usage: %s <coders> <burnout> <compile> <debug> "
			"<refactor> <compiles_req> <cooldown> <fifo|edf>\n", argv[0]);
		return (1);
	}
	if (!parse_numeric_args(config, argv))
	{
		fprintf(stderr, "Error: invalid numeric arguments or out of bounds.\n");
		return (1);
	}
	if (!parse_scheduler(config, argv[8]))
	{
		fprintf(stderr, "Error: scheduler must be 'fifo' or 'edf'.\n");
		return (1);
	}
	return (0);
}
