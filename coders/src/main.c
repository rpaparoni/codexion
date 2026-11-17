/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ripaparo <ripaparo@student.42madrid.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 13:26:45 by ripaparo          #+#    #+#             */
/*   Updated: 2026/11/17 15:45:00 by ripaparo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	main(int argc, char *argv[])
{
	t_config	config;

	if (parse_args(&config, argc, argv) != 0)
		return (1);
	return (0);
}
