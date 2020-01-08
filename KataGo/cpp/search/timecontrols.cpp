#include "../search/timecontrols.h"

TimeControls::TimeControls()
  :originalMainTime(1.0e30),
   increment(0.0),
   originalNumPeriods(0),
   numStonesPerPeriod(0),
   perPeriodTime(0.0),
   
   mainTimeLeft(1.0e30),
   inOvertime(false),
   numPeriodsLeftIncludingCurrent(0),
   numStonesLeftInPeriod(0),
   timeLeftInPeriod(0.0)
{}

TimeControls::~TimeControls()
{}

static double applyLagBuffer(double time, double lagBuffer) {
  if(time < 2.0 * lagBuffer)
    return time * 0.5;
  else
    return time - lagBuffer;
}

void TimeControls::getTime(const Board& board, const BoardHistory& hist, double lagBuffer, double& minTime, double& recommendedTime, double& maxTime) const {
  (void)hist;
  
  int boardArea = board.x_size * board.y_size;
  int numStonesOnBoard = 0;
  for(int y = 0; y < board.y_size; y++) {
    for(int x = 0; x < board.x_size; x++) {
      Loc loc = Location::getLoc(x,y,board.x_size);
      if(board.colors[loc] == C_BLACK || board.colors[loc] == C_WHITE)
        numStonesOnBoard++;
    }
  }

  //Very crude way to estimate game progress
  double approxTurnsLeft;
  {
    double typicalGameLengthToAllowFor = 0.60 * boardArea + 30.0;
    double minApproxTurnsLeft = 20.0 + 0.1 * boardArea;
    approxTurnsLeft = typicalGameLengthToAllowFor - numStonesOnBoard;
    if(approxTurnsLeft < minApproxTurnsLeft)
      approxTurnsLeft = minApproxTurnsLeft;
  }
  
  //We can be much more aggressive if we have overtime or an increment
  auto divideTimeEvenlyForGame = [approxTurnsLeft,this](double time) {
    double approxTurnsLeftForIncrement = approxTurnsLeft * 0.85;
    double approxTurnsLeftForByoYomi = approxTurnsLeft * 0.70;
    
    double base = time / approxTurnsLeft;
    double agginc = time / approxTurnsLeftForIncrement;
    double aggbyo = time / approxTurnsLeftForByoYomi;
    return base + std::max(std::min(agginc - base, 0.5 * increment),std::min(aggbyo - base, 0.5 * perPeriodTime));
  };
  
  //Initialize
  minTime = 0.0;
  recommendedTime = 0.0;
  maxTime = 0.0;
  
  //Fischer or absolute time handling
  if(increment > 0 || numPeriodsLeftIncludingCurrent <= 0) {
    if(inOvertime)
      throw StringError("TimeControls: inOvertime with Fischer or absolute time, inconsistent time control?");
    if(numPeriodsLeftIncludingCurrent != 0)
      throw StringError("TimeControls: numPeriodsLeftIncludingCurrent != 0 with Fischer or absolute time, inconsistent time control?");
      
    if(mainTimeLeft <= increment) {
      minTime = 0.0;
      recommendedTime = mainTimeLeft;
      maxTime = mainTimeLeft;
    }
    else {
      //Apply lagbuffer an extra time to the excessMainTime, ensuring we get extra buffering
      double excessMainTime = applyLagBuffer(mainTimeLeft - increment, lagBuffer);
      minTime = 0.0;
      recommendedTime = increment + divideTimeEvenlyForGame(excessMainTime);
      maxTime = increment + excessMainTime / 5.0;
    }
  }
  //Byo yomi time handling
  else {
    if(numStonesPerPeriod <= 0)
      throw StringError("TimeControls: numStonesPerPeriod <= 0 with byo-yomiish periods, inconsistent time control?");
    
    //Crudely treat all but the last 3 periods as main time.
    double effectiveMainTimeLeft = mainTimeLeft;
    bool effectivelyInOvertime = inOvertime;
    if(numPeriodsLeftIncludingCurrent > 3) {
      effectivelyInOvertime = false;
      if(!inOvertime) {
        effectiveMainTimeLeft += perPeriodTime * (numPeriodsLeftIncludingCurrent - 3);
      }
      else {
        effectiveMainTimeLeft += timeLeftInPeriod + perPeriodTime * (numPeriodsLeftIncludingCurrent - 4);
      }
    }

    if(!effectivelyInOvertime) {
      minTime = 0.0;
      recommendedTime = perPeriodTime / numStonesPerPeriod + divideTimeEvenlyForGame(effectiveMainTimeLeft);
      maxTime = perPeriodTime / (0.75 * numStonesPerPeriod + 0.25) + effectiveMainTimeLeft / 5.0;
    }
    else {
      if(numStonesLeftInPeriod < 1)
        throw StringError("TimeControls: numStonesLeftInPeriod < 1 while in overtime, inconsistent time control?");

      minTime = (numStonesLeftInPeriod <= 1) ? timeLeftInPeriod : 0.0;
      recommendedTime = timeLeftInPeriod / numStonesLeftInPeriod;
      maxTime = timeLeftInPeriod / (0.75 * numStonesLeftInPeriod + 0.25);
    }
  }

  //Lag buffer
  minTime = applyLagBuffer(minTime,lagBuffer);
  recommendedTime = applyLagBuffer(recommendedTime,lagBuffer);
  maxTime = applyLagBuffer(maxTime,lagBuffer);

  //Just in case
  if(minTime > maxTime)
    minTime = maxTime;
  if(recommendedTime > maxTime)
    recommendedTime = maxTime;
}
