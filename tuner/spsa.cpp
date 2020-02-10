/*
	This file is part of Vajolet.

    Vajolet is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Vajolet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Vajolet.  If not, see <http://www.gnu.org/licenses/>
*/

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "player.h"
#include "spsa.h"
#include "tournament.h"

struct runtimeVars{
	double value;
	double a;
	double c;
	double r;
	int delta;
};

struct variable{
	const std::string name;
	int SearchParameters::*par;
	const double startValue;
	const double minValue;
	const double maxValue;
	const double cEnd;
	const double rEnd;
	runtimeVars run;
};


SPSA::SPSA() {
	std::cout<<"SPSA"<<std::endl;
}

void SPSA::run() {
	
	std::random_device rd;
    std::mt19937 gen(rd());
	std::bernoulli_distribution d(0.5);
	
	std::vector<variable> pars = {
		{"razorMargin", &SearchParameters::razorMargin, 20000, 0, 40000, 5000, 0.0020, {}},
		{"razorMarginDepth", &SearchParameters::razorMarginDepth, 78, 0, 160, 30, 0.0020, {}},
		{"razorMarginCut", &SearchParameters::razorMarginCut, 20000, 0, 40000, 5000, 0.0020, {}}
	};
	
	for( auto& par: pars) {
		par.run.value = par.startValue;
	}
	
	for(int k = 1; k <= N; ++k) {
		std::cout<<"iteration "<<k<<std::endl;
		
		Player p1("p1");
		Player p2("p2");
		
		for( auto& par: pars) {
			std::cout<<"\tCalculating "<<par.name<<std::endl;
			double _c = par.cEnd * std::pow(N, gamma);
			double _aEnd = par.rEnd * std::pow(par.cEnd, 2.0);
			double _a = _aEnd * std::pow(A + N, alpha);
			std::cout<<"\t\tc:"<<_c<<" aEnd:"<<_aEnd<<" a:"<<_a<<std::endl;
			
			par.run.a = _a / std::pow(A + k, alpha);
			par.run.c = _c / std::pow(k, gamma);
			par.run.r = par.run.a / std::pow(par.run.c, 2.0);
			par.run.delta = d(gen) ? 1 : -1;
			std::cout<<"\t\ta:"<<par.run.a<<" c:"<<par.run.c<<" r:"<<par.run.r<<" delta:"<<par.run.delta<<std::endl;
			p1.getSearchParameters().*(par.par) = std::max(std::min(par.run.value + par.run.c * par.run.delta, par.maxValue), par.minValue);
			p2.getSearchParameters().*(par.par) = std::max(std::min(par.run.value - par.run.c * par.run.delta, par.maxValue), par.minValue);
		}
		
		std::cout<<"----------PARAMETERS--------"<<std::endl;	
		for( auto& par: pars) {
			std::cout<<par.name<<" "<<p1.getSearchParameters().*(par.par)<<" "<<p2.getSearchParameters().*(par.par)<<std::endl;
		}
		
		Tournament t("tournament" + std::to_string(k)+".pgn", p1, p2);
		auto res = t.play();
		std::cout<<"Tournament Result: "<<static_cast<int>(res)<<std::endl;
		
		for( auto& par: pars) {
			par.run.value = std::max(std::min(par.run.value + par.run.r * par.run.c * static_cast<int>(res) * par.run.delta, par.maxValue), par.minValue);
			std::cout<<par.name<<":"<<par.run.value<<std::endl;
		}
		
		
	}

}