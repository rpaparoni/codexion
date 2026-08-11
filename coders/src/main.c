/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ripaparo <ripaparo@student.42madrid.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 13:26:45 by ripaparo          #+#    #+#             */
/*   Updated: 2026/08/11 16:53:27 by ripaparo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	main(int argc, char *argv[])
{
    if (argc == 7)
	    parsing(argv);
    else
        printf("error");
}
