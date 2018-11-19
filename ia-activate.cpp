/*
Copyright (C) 2011 Georgia Institute of Technology

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A mARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* Generates square pulse current commands.
*/

#include <ia-activate.h>

extern "C" Plugin::Object *createRTXIPlugin(void) {
	return new IAact();
}

static DefaultGUIModel::variable_t vars[] =
{
	{ "Vin", "", DefaultGUIModel::INPUT, },
	{ "Output_To_OTA", "", DefaultGUIModel::OUTPUT, },
	{ "Downtime (ms)", "The time iout is 0 between depolarization and hyperpolarization", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Hyperpolarization Time (ms)", "On time of the step during a single cycle",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Depolarization Time (ms)", "Time current is at depolarized value",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Current Range Start (pA)", "Starting current of the steps",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Current Range End (pA)", "Ending current of the steps", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, },
	{ "Increments", "How many steps to take between min and max",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	{ "Cycles (#)", "How many times to repeat the protocol",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER, },
	
	{ "Max Amplitude (pA)", "Value of the maximum amplitude",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	
	{ "Offset (mA)", "DC offset to add", DefaultGUIModel::PARAMETER
		| DefaultGUIModel::DOUBLE, }, 
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

IAact::IAact(void) : DefaultGUIModel("IA Activation", ::vars, ::num_vars), dt(RT::System::getInstance()->getPeriod() * 1e-6), downtime(1000), rStart(0), rEnd(-160.0), Nsteps(6), Ncycles(1), hyperpolTime(150), maxAmp(150.0), depolTime(1000), offset(0.0) {
	setWhatsThis("<p><b>I-Step:</b><br>This module generates a series of currents in a designated range followed by a fixed maximum current.</p>");
	createGUI(vars, num_vars);
	update(INIT);
	refresh();
	resizeMe();
}

IAact::~IAact(void) {}

void IAact::execute(void) {
	V = input(0);
	
	Iout = offset;
	if (cycle < Ncycles) {
		
		//Do all time keeping in seconds.
		if (step < Nsteps) {
			if (age >= 0 && age < ((hyperpolTime / 1000)) - EPS) {
				Iout = Iout + rStart -(step*deltaI);
				age += dt / 1000;
			}
			else if(interage<(depolTime/1000)) {
				Iout = maxAmp;
				interage += dt / 1000;
			}
			
			else{
				age += dt/1000;
			}
			
			if (age >= ((downtime/1000)+(hyperpolTime/1000) - EPS)) {
				step++;
				interage = 0;
				age = 0;
			}
			
		}
		if (step == Nsteps) {
			cycle++;
			step = 0;
		}
	}
	Output_to_mV = Iout * .5e-3;
	output(0) = Output_to_mV;
}

void IAact::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("Downtime (ms)", downtime);
			setParameter("Hyperpolarization Time (ms)", hyperpolTime);
                        setParameter("Depolarization Time (ms)", depolTime);
			setParameter("Current Range Start (pA)", rStart);
			setParameter("Current Range End (pA)", rEnd);
			setParameter("Increments", Nsteps);
			setParameter("Cycles (#)", Ncycles);
			setParameter("Max Amplitude (pA)", maxAmp);
			setParameter("Offset (mA)", offset);
			break;

		case MODIFY:
			downtime = getParameter("Downtime (ms)").toDouble();
			rStart = getParameter("Current Range Start (pA)").toDouble();
			rEnd = getParameter("Current Range End (pA)").toDouble();
			Nsteps = getParameter("Increments").toInt();
			Ncycles = getParameter("Cycles (#)").toInt();
			hyperpolTime = getParameter("Hyperpolarization Time (ms)").toDouble();
			maxAmp = getParameter("Max Amplitude (pA)").toDouble();
			depolTime = getParameter("Depolarization Time (ms)").toDouble();
			offset = getParameter("Offset (mA)").toDouble();
			break;

		case PAUSE:
			output(0) = 0;
	
		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-6;
	
		default:
			break;
	}
	
	// Some Error Checking for fun
	
	if ((downtime/1000) <= 0) {
		downtime = 0;
		setParameter("Period (sec)", downtime);
	}
	
	if (rStart < rEnd) {
		rStart = rEnd;
		setParameter("Min Amp (mA)", rEnd);
		setParameter("Max Amp (mA)", rStart);
	}
	
	if (Ncycles < 1) {
		Ncycles = 1;
		setParameter("Cycles (#)", Ncycles);
	}
	
	if (Nsteps < 0) {
		Nsteps = 0;
		setParameter("Increments", Nsteps);
	}
	
	if (hyperpolTime < 0 || hyperpolTime > 100000) {
		hyperpolTime = 0;
		setParameter("Hyperpolarization Time (ms)", hyperpolTime);
	}
	//Define deltaI based on mArams
	if (Nsteps > 1) {
		deltaI = (rStart - rEnd) / (Nsteps - 1);
	}
	else {
		deltaI = 0;
	}
	
	//Initialize counters
	age = 0;
	step = 0;
	cycle = 0;
	interage = 0;	
}
