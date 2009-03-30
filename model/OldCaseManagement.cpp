/* This file is part of OpenMalaria.
 * 
 * Copyright (C) 2005,2006,2007,2008 Swiss Tropical Institute and Liverpool School Of Tropical Medicine
 * 
 * OpenMalaria is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "OldCaseManagement.h"
#include "inputData.h"
#include "global.h"
#include <iostream>
#include <limits>

const int OldCaseManagement::SEQUELAE_AGE_BOUND[NUM_SEQUELAE_AGE_GROUPS] = { 1, 10 };
double OldCaseManagement::_oddsRatioThreshold;
bool OldCaseManagement::_noMortality;
std::vector<double> OldCaseManagement::_inputAge;
std::vector<double> OldCaseManagement::_caseFatalityRate;
double OldCaseManagement::probGetsTreatment[3];
double OldCaseManagement::probParasitesCleared[3];
double OldCaseManagement::cureRate[3];
double OldCaseManagement::probSequelaeTreated[2];
double OldCaseManagement::probSequelaeUntreated[2];

void OldCaseManagement::init (){
  _oddsRatioThreshold = exp(getParameter(Params::LOG_ODDS_RATIO_CF_COMMUNITY));
  
  setParasiteCaseParameters ();
  
  const ByAgeItems::ItemSequence& items = getHealthSystem().getPSequelaeInpatient().getItem();
  for (int agegrp=0; agegrp < NUM_SEQUELAE_AGE_GROUPS; agegrp++) {
    for (size_t i = 0; i < items.size(); i++) {
      if (items[i].getMaxAgeYrs() > SEQUELAE_AGE_BOUND[agegrp]) {
        probSequelaeTreated[agegrp] =
        probSequelaeUntreated[agegrp] = items[i].getValue();
        goto gotItem;
      }
    }
    cerr << "In scenario.xml: healthSystem: pSequelaeInpatient: expected item with maxAgeYrs > 10" << endl;
    throw 0;
    gotItem:;	// alternative to for ... break ... else
  }
  
  readCaseFatalityRatio();
}

OldCaseManagement::OldCaseManagement()
{}

OldCaseManagement::~OldCaseManagement(){
}

void OldCaseManagement::readCaseFatalityRatio(){
  const AgeGroups::GroupSequence& xmlGroupSeq = getHealthSystem().getCFR().getGroup();
  
  int numOfGroups = xmlGroupSeq.size();
  _inputAge.resize(numOfGroups+1);
  _caseFatalityRate.resize(numOfGroups+1);
  
  for (int i=0;i<numOfGroups; i++) {
    const Group& xmlGroup = xmlGroupSeq[i];
    _inputAge[i] = xmlGroup.getLowerbound();
    _caseFatalityRate[i] = xmlGroup.getCfr();
  }
  
  //NOTE: need to make sure _inputAge[0] == 0 (or less)
  _inputAge[0] = 0;
  
  //Use a high number for the upper bound.
  _inputAge[numOfGroups] = numeric_limits<double>::infinity();
  //CFR is constant for everyone above the highest upperbound
  _caseFatalityRate[numOfGroups] = _caseFatalityRate[numOfGroups-1];
  _noMortality = (numOfGroups == 1) && ( _caseFatalityRate[0] == 0);
}

double OldCaseManagement::getCommunityCaseFatalityRate(double caseFatalityRatio) {
  double x = caseFatalityRatio * _oddsRatioThreshold;
  return x / (1 - caseFatalityRatio + x);
}

int OldCaseManagement::getNextRegimen(int simulationTime, int diagnosis, int tLastTreated, int regimen) {
  if (diagnosis == Diagnosis::SEVERE_MALARIA)
    return 3;
  
  if (tLastTreated > (simulationTime-caseManagementMemory))
    return 2;
  
  return 1;
}

double OldCaseManagement::getProbabilityGetsTreatment(int regimen) {
  return probGetsTreatment[regimen];
}

double OldCaseManagement::getProbabilityParasitesCleared(int regimen) {
  return probParasitesCleared[regimen];
}

double OldCaseManagement::getCureRate(int regimen) {
  return cureRate[regimen];
}

double OldCaseManagement::getProbabilitySequelaeTreated(int ageGroup) {
  return probSequelaeTreated[ageGroup];
}

double OldCaseManagement::getProbabilitySequelaeUntreated(int ageGroup) {
  return probSequelaeUntreated[ageGroup];
}

double OldCaseManagement::caseFatality(double ageYears) {
  // NOTE: assume ageYears >= 0 and _inputAge[0] <= 0
  if (_noMortality)
    return 0.0;
  
  size_t i = 0;
  while(_inputAge[i] <= ageYears)
    ++i;
  
  // _inputAge[i-1] <= ageYears < _inputAge[i]
  double a0 = _inputAge[i-1];
  double f0 = _caseFatalityRate[i-1];
  return (ageYears - a0) / (_inputAge[i] - a0) * (_caseFatalityRate[i]-f0) + f0;
}


double getHealthSystemACRByName (const TreatmentDetails& td, string firstLineDrug) {
  if (firstLineDrug == "CQ")
    return td.getCQ().present() ? td.getCQ().get().getValue() : 0;
  else if (firstLineDrug == "SP")
    return td.getSP().present() ? td.getSP().get().getValue() : 0;
  else if (firstLineDrug == "AQ")
    return td.getAQ().present() ? td.getAQ().get().getValue() : 0;
  else if (firstLineDrug == "SPAQ")
    return td.getSPAQ().present() ? td.getSPAQ().get().getValue() : 0;
  else if (firstLineDrug == "ACT")
    return td.getACT().present() ? td.getACT().get().getValue() : 0;
  else if (firstLineDrug == "QN")
    return td.getQN().present() ? td.getQN().get().getValue() : 0;
  else if (firstLineDrug == "selfTreatment")
    return td.getSelfTreatment().getValue();
  else {
    cerr << "healthSystem.drugRegimen->firstLine has bad value" << endl;
    throw 0;
  }
}

void OldCaseManagement::setParasiteCaseParameters () {
  const HealthSystem& healthSystem = getHealthSystem();
  
  
  // --- calculate cureRate ---
  
  //We get the ACR depending on the name of firstLineDrug.
  cureRate[0] = getHealthSystemACRByName (healthSystem.getInitialACR(),
                                          healthSystem.getDrugRegimen().getFirstLine());
  
  //Calculate curerate 0
  double pSeekOfficialCareUncomplicated1 = healthSystem.getPSeekOfficialCareUncomplicated1().getValue();
  double pSelfTreatment = healthSystem.getPSelfTreatUncomplicated().getValue();
  if (pSeekOfficialCareUncomplicated1 + pSelfTreatment > 0){
    double cureRateSelfTreatment = healthSystem.getInitialACR().getSelfTreatment().getValue();
    
    cureRate[0] = (cureRate[0] * pSeekOfficialCareUncomplicated1
        + cureRateSelfTreatment * pSelfTreatment)
       / (pSeekOfficialCareUncomplicated1+pSelfTreatment);
  }

  cureRate[1] = getHealthSystemACRByName (healthSystem.getInitialACR(),
                                          healthSystem.getDrugRegimen().getSecondLine());

  cureRate[2] = getHealthSystemACRByName (healthSystem.getInitialACR(),
                                          healthSystem.getDrugRegimen().getInpatient());
  
  
  // --- calculate probGetsTreatment ---
  
  probGetsTreatment[0] = healthSystem.getPSeekOfficialCareUncomplicated1().getValue() + healthSystem.getPSelfTreatUncomplicated().getValue();
  probGetsTreatment[1] = healthSystem.getPSeekOfficialCareUncomplicated2().getValue();
  probGetsTreatment[2] = healthSystem.getPSeekOfficialCareSevere().getValue();
  
  
  // --- calculate probParasitesCleared ---
  
  string firstLineDrug = healthSystem.getDrugRegimen().getFirstLine();
  string secondLineDrug = healthSystem.getDrugRegimen().getSecondLine();	
  pSeekOfficialCareUncomplicated1 = healthSystem.getPSeekOfficialCareUncomplicated1().getValue();

  double complianceFirstLine = getHealthSystemACRByName (healthSystem.getCompliance(), firstLineDrug);
  double complianceSecondLine = getHealthSystemACRByName (healthSystem.getCompliance(), secondLineDrug);

  double cureRateFirstLine = getHealthSystemACRByName (healthSystem.getInitialACR(), firstLineDrug);
  double cureRateSecondLine = getHealthSystemACRByName (healthSystem.getInitialACR(), secondLineDrug);

  double nonCompliersEffectiveFirstLine = getHealthSystemACRByName (healthSystem.getNonCompliersEffective(), firstLineDrug);
  double nonCompliersEffectiveSecondLine = getHealthSystemACRByName (healthSystem.getNonCompliersEffective(), secondLineDrug);

  pSelfTreatment = healthSystem.getPSelfTreatUncomplicated().getValue();
  double complianceSelfTreatment = healthSystem.getCompliance().getSelfTreatment().getValue();
  double cureRateSelfTreatment = healthSystem.getInitialACR().getSelfTreatment().getValue();
  
  //calculate probParasitesCleared 0
  if ((pSeekOfficialCareUncomplicated1 + pSelfTreatment) > 0){
    probParasitesCleared[0] = (pSeekOfficialCareUncomplicated1
        * (complianceFirstLine * cureRateFirstLine
         + (1 - complianceFirstLine) * nonCompliersEffectiveFirstLine)
       + pSelfTreatment
        * (complianceSelfTreatment * cureRateSelfTreatment
         + (1 - complianceSelfTreatment) * nonCompliersEffectiveFirstLine))
      / (pSeekOfficialCareUncomplicated1 + pSelfTreatment);
  } else {
    probParasitesCleared[0] = 0;
  }
  
  //calculate probParasitesCleared 1
  probParasitesCleared[1] = complianceSecondLine * cureRateSecondLine
      + (1 - complianceSecondLine)
      * nonCompliersEffectiveSecondLine;

  //calculate probParasitesCleared 2 : cool :)
  probParasitesCleared[2] = 0;
}


// -----  checkpointing  -----

void OldCaseManagement::read(istream& in) {
  in >> _latestEvent;
}
void OldCaseManagement::write(ostream& out) const {
  out << _latestEvent;
}