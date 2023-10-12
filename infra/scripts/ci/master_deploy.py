import argparse

from hestia_ci import CIManager, GitlabClient, BuildInfo

DeployArgs = CIManager.DeployArgs

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--bot_token', type=str, required=True)
    BuildInfo.add_arguments(parser)
    DeployArgs.add_arguments(parser)
    
    args = parser.parse_args()


    ci_manager = CIManager(build_info=BuildInfo(**vars(args)), 
                           client=GitlabClient(private_token=args.bot_token)) 
    
    ci_manager.deploy_release(DeployArgs(**vars(args)))