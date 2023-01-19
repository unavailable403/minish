/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ergrigor < ergrigor@student.42yerevan.am > +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/19 17:42:37 by ergrigor          #+#    #+#             */
/*   Updated: 2023/01/19 20:11:06 by ergrigor         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "main.h"

t_global	*g_lobal;

int	mshloop(struct termios *gago, char *cmd_line, int status)
{
	while (1)
	{
		if (tcsetattr(0, TCSANOW, gago) < 0)
			ft_putstr_fd("Error\n", 2);
		signal_call(1);
		cmd_line = readline("Say - Hello myalmo > ");
		if (cmd_line == NULL)
			return (ft_putstr_fd("exit\n", 1), 1);
		if (empty_line(cmd_line) != 1)
		{
			add_history(cmd_line);
			g_lobal->tokens = lexer(cmd_line);
			status = lex_analyser(g_lobal->tokens);
			if (status == 0)
			{
				make_struct();
			}
			free(cmd_line);
			destroy_struct();
		}
	}
	return (0);
}

int	main(int argc, char **argv, char **_env)
{
	struct termios	gago;
	char			*cmd_line;
	char			**msh_env;
	int				status;

	(void)argc;
	(void)argv;
	g_lobal = ft_calloc(sizeof(t_global), 1);
	g_lobal->env = pars_env(_env);
	msh_env = get_arr_env(g_lobal->env);
	_env = msh_env;
	cmd_line = NULL;
	status = 0;
	rl_catch_signals = 0;
	if (tcgetattr(0, &gago) < 0)
		ft_putstr_fd("Error\n", 2);
	return (mshloop(&gago, cmd_line, status));
}
