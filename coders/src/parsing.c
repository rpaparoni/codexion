/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ripaparo <ripaparo@student.42madrid.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 16:36:56 by ripaparo          #+#    #+#             */
/*   Updated: 2026/08/11 16:56:23 by ripaparo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int  parsing(char *argv[])
{
    int i;

    i = 0;
    while(argv[i])
    {
        if (i != 7)
            atoi(argv[i]);
    }
    return
}
