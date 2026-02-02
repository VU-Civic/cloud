#include <aws/sqs/model/PurgeQueueRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include "sqs.h"

AwsSQS::AwsSQS(const char* __restrict queueURL) : sqsClient(AWS::getSQSClient()), queueURL(queueURL) {}
AwsSQS::~AwsSQS(void) {}

bool AwsSQS::sendMessage(const char* __restrict message)
{
  // Send the specified message to the SQS queue
  Aws::SQS::Model::SendMessageRequest sendMessageRequest;
  sendMessageRequest.SetQueueUrl(queueURL);
  sendMessageRequest.SetMessageBody(message);
  const auto outcome = sqsClient->SendMessage(sendMessageRequest);
  if (!outcome.IsSuccess()) logger.log(Logger::ERROR, "AWS SQS: SendMessage failed on Queue '%s' with error: %s\n", queueURL.c_str(), outcome.GetError().GetMessage().c_str());
  return outcome.IsSuccess();
}

std::tuple<bool, std::string, std::string> AwsSQS::receiveMessage(int secondsToWait)
{
  // Receive a message from the SQS queue
  Aws::SQS::Model::ReceiveMessageRequest receiveMessageRequest;
  receiveMessageRequest.SetQueueUrl(queueURL);
  receiveMessageRequest.SetMaxNumberOfMessages(1);
  receiveMessageRequest.SetWaitTimeSeconds(secondsToWait);
  const auto outcome = sqsClient->ReceiveMessage(receiveMessageRequest);
  if (outcome.IsSuccess())
  {
    if (outcome.GetResult().GetMessages().empty())
    {
      logger.log(Logger::WARNING, "AWS SQS: No message received from Queue '%s' within %d seconds\n", queueURL.c_str(), secondsToWait);
      return std::make_tuple(false, "", "");
    }
    else
    {
      const auto& message = outcome.GetResult().GetMessages().front();
      return std::make_tuple(true, message.GetReceiptHandle(), message.GetBody());
    }
  }
  else
    logger.log(Logger::ERROR, "AWS SQS: ReceiveMessage failed on Queue '%s' with error: %s\n", queueURL.c_str(), outcome.GetError().GetMessage().c_str());
  return std::make_tuple(false, "", "");
}

bool AwsSQS::deleteMessage(const char* __restrict receiptHandle)
{
  // Delete the specified message from the SQS queue
  Aws::SQS::Model::DeleteMessageRequest deleteMessageRequest;
  deleteMessageRequest.SetQueueUrl(queueURL);
  deleteMessageRequest.SetReceiptHandle(receiptHandle);
  const auto outcome = sqsClient->DeleteMessage(deleteMessageRequest);
  if (!outcome.IsSuccess()) logger.log(Logger::ERROR, "AWS SQS: DeleteMessage failed on Queue '%s' with error: %s\n", queueURL.c_str(), outcome.GetError().GetMessage().c_str());
  return outcome.IsSuccess();
}

bool AwsSQS::purgeQueue(void)
{
  // Purge all messages from the specified SQS queue
  Aws::SQS::Model::PurgeQueueRequest purgeQueueRequest;
  purgeQueueRequest.SetQueueUrl(queueURL);
  const auto outcome = sqsClient->PurgeQueue(purgeQueueRequest);
  if (!outcome.IsSuccess()) logger.log(Logger::ERROR, "AWS SQS: PurgeQueue failed on Queue '%s' with error: %s\n", queueURL.c_str(), outcome.GetError().GetMessage().c_str());
  return outcome.IsSuccess();
}
