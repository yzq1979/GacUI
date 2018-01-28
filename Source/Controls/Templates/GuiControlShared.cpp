#include "GuiCommonTemplates.h"
#include "../../GraphicsComposition/GuiGraphicsCompositionBase.h"
#include "../GuiWindowControls.h"

namespace vl
{
	namespace presentation
	{
		namespace controls
		{
			using namespace reflection::description;
			using namespace compositions;

/***********************************************************************
GuiComponent
***********************************************************************/
			
			GuiComponent::GuiComponent()
			{
			}

			GuiComponent::~GuiComponent()
			{
			}

			void GuiComponent::Attach(GuiInstanceRootObject* rootObject)
			{
			}

			void GuiComponent::Detach(GuiInstanceRootObject* rootObject)
			{
			}

/***********************************************************************
GuiInstanceRootObject
***********************************************************************/

			class RootObjectTimerCallback : public Object, public IGuiGraphicsTimerCallback
			{
			public:
				GuiControlHost*					controlHost;
				GuiInstanceRootObject*			rootObject;
				bool							alive = true;

				RootObjectTimerCallback(GuiInstanceRootObject* _rootObject, GuiControlHost* _controlHost)
					:rootObject(_rootObject)
					, controlHost(_controlHost)
				{
				}

				bool Play()override
				{
					return alive;
				}
			};

			void GuiInstanceRootObject::OnControlHostForInstanceChanged()
			{
				auto controlHost = GetControlHostForInstance();
				if (timerCallback && timerCallback->controlHost != controlHost)
				{
					timerCallback->alive = false;
					timerCallback = nullptr;

					FOREACH(Ptr<IGuiAnimation>, animation, runningAnimations)
					{
						animation->Pause();
					}
				}

				if (controlHost && !timerCallback)
				{
					timerCallback = new RootObjectTimerCallback(this, controlHost);
					controlHost->GetTimerManager()->AddCallback(timerCallback);

					FOREACH(Ptr<IGuiAnimation>, animation, runningAnimations)
					{
						animation->Resume();
					}
					StartPendingAnimations();
				}
			}

			void GuiInstanceRootObject::StartPendingAnimations()
			{
				FOREACH(Ptr<IGuiAnimation>, animation, pendingAnimations)
				{
					animation->Start();
				}

				CopyFrom(runningAnimations, pendingAnimations, true);
				pendingAnimations.Clear();
			}

			GuiInstanceRootObject::GuiInstanceRootObject()
			{
			}

			GuiInstanceRootObject::~GuiInstanceRootObject()
			{
				if (timerCallback)
				{
					timerCallback->alive = false;
				}
			}

			void GuiInstanceRootObject::FinalizeInstance()
			{
				if (!finalized)
				{
					finalized = true;

					FOREACH(Ptr<IValueSubscription>, subscription, subscriptions)
					{
						subscription->Close();
					}
					FOREACH(GuiComponent*, component, components)
					{
						component->Detach(this);
					}

					subscriptions.Clear();
					for (vint i = 0; i<components.Count(); i++)
					{
						delete components[i];
					}
					components.Clear();
				}
			}

			bool GuiInstanceRootObject::IsFinalized()
			{
				return finalized;
			}

			void GuiInstanceRootObject::FinalizeInstanceRecursively(templates::GuiTemplate* thisObject)
			{
				if (!finalized)
				{
					NotifyFinalizeInstance(thisObject);
				}
			}

			void GuiInstanceRootObject::FinalizeInstanceRecursively(GuiCustomControl* thisObject)
			{
				if (!finalized)
				{
					NotifyFinalizeInstance(thisObject);
				}
			}

			void GuiInstanceRootObject::FinalizeInstanceRecursively(GuiControlHost* thisObject)
			{
				if (!finalized)
				{
					NotifyFinalizeInstance(thisObject);
				}
			}

			void GuiInstanceRootObject::FinalizeGeneralInstance(GuiInstanceRootObject* thisObject)
			{
			}

			void GuiInstanceRootObject::SetResourceResolver(Ptr<GuiResourcePathResolver> resolver)
			{
				resourceResolver = resolver;
			}

			Ptr<DescriptableObject> GuiInstanceRootObject::ResolveResource(const WString& protocol, const WString& path, bool ensureExist)
			{
				Ptr<DescriptableObject> object;
				if (resourceResolver)
				{
					object = resourceResolver->ResolveResource(protocol, path);
				}
				if (ensureExist && !object)
				{
					throw ArgumentException(L"Resource \"" + protocol + L"://" + path + L"\" does not exist.");
				}
				return object;
			}

			Ptr<description::IValueSubscription> GuiInstanceRootObject::AddSubscription(Ptr<description::IValueSubscription> subscription)
			{
				CHECK_ERROR(finalized == false, L"GuiInstanceRootObject::AddSubscription(Ptr<IValueSubscription>)#Cannot add subscription after finalizing.");
				if (subscriptions.Contains(subscription.Obj()))
				{
					return nullptr;
				}
				else
				{
					subscriptions.Add(subscription);
					subscription->Open();
					subscription->Update();
					return subscription;
				}
			}

			void GuiInstanceRootObject::UpdateSubscriptions()
			{
				FOREACH(Ptr<IValueSubscription>, subscription, subscriptions)
				{
					subscription->Update();
				}
			}

			bool GuiInstanceRootObject::AddComponent(GuiComponent* component)
			{
				CHECK_ERROR(finalized == false, L"GuiInstanceRootObject::AddComponent(GuiComponent*)#Cannot add component after finalizing.");
				if(components.Contains(component))
				{
					return false;
				}
				else
				{
					components.Add(component);
					component->Attach(this);
					return true;
				}
			}

			bool GuiInstanceRootObject::AddControlHostComponent(GuiControlHost* controlHost)
			{
				return AddComponent(new GuiObjectComponent<GuiControlHost>(controlHost));
			}

			bool GuiInstanceRootObject::AddAnimation(Ptr<IGuiAnimation> animation)
			{
				CHECK_ERROR(finalized == false, L"GuiInstanceRootObject::AddAnimation(Ptr<IGuiAnimation>)#Cannot add animation after finalizing.");
				if (runningAnimations.Contains(animation.Obj()) || pendingAnimations.Contains(animation.Obj()))
				{
					return false;
				}
				else
				{
					pendingAnimations.Add(animation);

					if (auto controlHost = GetControlHostForInstance())
					{
						if (!timerCallback)
						{
							timerCallback = new RootObjectTimerCallback(this, controlHost);
							controlHost->GetTimerManager()->AddCallback(timerCallback);
							StartPendingAnimations();
						}
					}

					return true;
				}
			}
		}
	}
}